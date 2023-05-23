#include "Epoll.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Socket.hpp"
#include "define.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <wait.h>
#include <cstring>

/* ConnScoketクラスと同じアウトラインを持つ
  CgiSocket(ConnSocket *conn_socket);
  ~CgiSocket();
  CgiSocket *CreatCgiProcess();
  int OnWritable(Epoll *epoll);
  int OnReadable(Epoll *epoll);
  int ProcessSocket(Epoll *epoll, void *data);
*/

// CGI実行を要求したHTTPリクエストとクライアントの登録、そのクライアントのconfigを登録しておく
CgiSocket::CgiSocket(ConnSocket &conn_socket, Request &http_request,
                     const ConfVec &config)
    : ASocket(config), conn_socket_(&conn_socket), http_request_(http_request) {
  this->send_buffer_.AddString(http_request_.GetRequestMessage().body.c_str());
};

// このクラス独自のデストラクタ内の処理は特にないメンバ変数自身のデストラクタが暗黙に呼ばれることに任せる
CgiSocket::~CgiSocket() {
  int status;

  // 子プロセスが終了していない場合に備え、WNOHANGを使う
  int wait_res = waitpid(pid_, &status, WNOHANG);

  switch (wait_res) {
  case 0: // 子プロセスが終了していないのでkill()で終了させて、終了ステータスを回収する
    if (kill(pid_, SIGKILL) < 0) {
      std::cerr << "Keep Running Error: kill" << std::endl;
    }
    if (waitpid(pid_, &status, WNOHANG) < 0) {
      std::cerr << "Keep Running Error: waitpid" << std::endl;
    }
    break;

  case -1: // waitpid()でエラーが発生した
    std::cerr << "Keep Running Error: waitpid" << std::endl;
    break;

  default: // 子プロセスが終了していて、終了ステータスを回収できた
    break;
  }
}

// CGIスクリプト実行用のプロセスを作成し、実行する
// 標準入出力での通信用にUNIXドメインのペアソケットを作成する
// 親プロセスであるサーバー側は、そのソケットをepollに登録し、I/O多重化のうえで通信を行う
// epollへの登録のために、ASocketクラスから派生したCgiSocketクラスをポインタで返し、アップキャストでの多相性を利用してEpollクラスのインターフェースを利用する
// エラー時にはNULLを返す
CgiSocket *CgiSocket::CreatCgiProcess() {
  // UNIXドメインのペアソケットを作成
  int cgi_socket[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, cgi_socket) < 0) {
    std::cerr << "Keep Running Error: socketpair" << std::endl;
    return NULL;
  }
  int parent_sock = cgi_socket[0];
  int child_sock = cgi_socket[1];

  // 子プロセスを作成
  this->pid_ = fork();
  if (pid_ < 0) {
    std::cerr << "Keep Running Error: fork" << std::endl;
    if (close(parent_sock) < 0) {
      std::cerr << "Keep Running Error: close" << std::endl;
    }
    if (close(child_sock) < 0) {
      std::cerr << "Keep Running Error: close" << std::endl;
    }
    return NULL; // HTTPクライアントには500エラーを返してもらう
  } else if (pid_ == 0) { // 子プロセスでの処理
    // 子プロセスでのCGIスクリプト実行の前処理
    this->SetSocket(child_sock, parent_sock);
    char **env = this->SetMetaVariables();
    char **argv = this->SetArgv();
    this->SetCurrentDir(
        this->http_request_.GetContext().resource_path.server_path);

    // CGIスクリプトを実行
    if (execve(
            this->http_request_.GetContext().resource_path.server_path.c_str(),
            argv, env) < 0) {
      // 子プロセス内でのエラーは例外を投げてすぐ子プロセスを終了させる
      throw std::runtime_error(
          "Keep Running Error: close in CGI script procces");
    }
  }

  // 親プロセスでの処理
  // 親プロセス側のソケットを閉じる
  if (close(child_sock) < 0) {
    std::cerr << "Keep Running Error: close" << std::endl;
    if (close(parent_sock) < 0) {
      std::cerr << "Keep Running Error: close" << std::endl;
    }
    // 子プロセスを終了させる
    if (kill(pid_, SIGTERM) < 0) {
      std::cerr << "Keep Running Error: kill" << std::endl;
    }
    return NULL; // HTTPクライアントには500エラーを返してもらう
  }

  // 親プロセス側のソケットFDを設定
  SetFd(parent_sock);

  // CGIリクエスト書き込みまでの制限時間の起算点を設定
  this->last_event_.out_time = time(NULL);

  // CgiSocketを返り値にしてEPOLLにEPOLLOUTでADDしてもらう
  return this;
}

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int CgiSocket::OnReadable(Epoll *epoll) {
  // UNIXドメインソケットからCGIスクリプトの出力を受け取る
  int recv_result = recv_buffer_.ReadSocket(this->GetFd());

  switch (recv_result) {
  case 0: // CGIレスポンスを受信しバッファにためた。あえてまだParseはしない。finパケットを受信し、EOFを読み込んだら（case
          // -1）Parseする
    break;

  case -1: // finパケットを受信し、CGIスクリプトが終了したのでCGIレスポンスをParseし、HTTPレスポンスを作成しdequeに追加して、EpollにEPOLLOUTでADDしHTTPクライアントへのレスポンス送信に備える
    // parseし、HTTPレスポンスを作成する
    Response http_response = this->ParseCgiResponse();

    // HTTPレスポンスをdequeに追加する
    this->conn_socket_->PushResponse(http_response);

    // EpollにEPOLLOUT追加でADDする
    uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLET;
    epoll->Add(this->conn_socket_, event_mask);

    return FAILURE;
    // FAILUREを返せば、子プロセスのkill()とwaitpid()が、CgiSocketのデストラクタで行われる
    // UNIXドメインのFDのclose()が、基底クラスのASocketのデストラクタで行われる
    // OnWritable()の呼び出し元で、さらにFAILUREを返し、EPOLLの登録も解除される
  }

  return SUCCESS;
}

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int CgiSocket::OnWritable(Epoll *epoll) {
  // send_buffer_の内容をUNIXソケットを通じてCGIスクリプトに書き込む
  int send_result;
  if (send_buffer_.GetBuffSize()) {
    send_result = send_buffer_.SendSocket(this->GetFd());
  } else {
    send_result = 1; // 送信完了扱いとする
  }

  switch (send_result) {

  case 0: {// 送信未完了
    // EPOLLOUTのイベント登録を維持しつつ、次回のEPOLLOUTを発火を待つ
    last_event_.out_time = time(NULL); // 現在時間に更新
    break;
  }
  case 1: { // 送信完了
    // EPOLLOUTのイベント登録を解除し、EPOLLINのイベント登録を行う
    uint32_t event_mask = EPOLLIN | EPOLLRDHUP | EPOLLET;
    epoll->Mod(this->GetFd(), event_mask);
    // UNIXドメインソケットの全二重通信の送信側を閉じるこれにより、CGIスクリプト側のread()がEOFを検知することができる
    if (shutdown(this->GetFd(), SHUT_WR) < 0) {
      std::cerr << "Keep Running Error: shutdown" << std::endl;
    }
    last_event_.out_time = -1; // タイムアウトを無効化
    break;
  }
  default: { // (-1) エラー
    return FAILURE;
    // FAILUREを返せば、子プロセスのkill()とwaitpid()が、CgiSocketのデストラクタで行われる
    // UNIXドメインのFDのclose()が、基底クラスのASocketのデストラクタで行われる
    // OnWritable()の呼び出し元で、さらにFAILUREを返し、EPOLLの登録も解除される
    break;
  }
  }

  return SUCCESS;
}

// SUCCESS: 引き続きsocketを利用
// FAILURE: socketを閉じる。異常シナリオでも、FAILUREを使う
// FILURE:
// あわせて、CgiSocketのデストラクタが呼ばれ、子プロセスkill()し、waitpid()して、子プロセスの終了やゾンビプロセスの発生を防ぐ
int CgiSocket::ProcessSocket(Epoll *epoll, void *data) {
  // clientからの通信を処理
  // std::cout << "Socket: " << fd_ << std::endl;
  uint32_t event_mask = *(static_cast<uint32_t *>(data));
  if (event_mask & EPOLLERR || event_mask & EPOLLHUP) {
    // エラーイベント keep runningエラーを出力はしつつ、クライアントには500
    // Internal Server Errorを返す
    std::cerr << "Keep Running Error: EPOLLERR or EPOLLHUP" << std::endl;

    // TODO: HTTP 500 Internal Server Error を、クライアントのresponse_
    // dequeに追加する

    return FAILURE;
  }
  if (event_mask & EPOLLIN) {
    // 受信(Todo: OnReadable(0))
    std::cout << fd_ << ": EPOLLIN" << std::endl;
    if (OnReadable(epoll) == FAILURE) {
      return FAILURE;
    }
  }
  if (event_mask & EPOLLOUT) {
    // send_buffer_からの送信。HTTPリクエストにエンティティボディが無い場合はsend_buffer_には何も入っていない。ただし、一度send()のステップを踏んで、送信完了と合流する
    std::cout << fd_ << ": EPOLLOUT" << std::endl;
    if (OnWritable(epoll) == FAILURE) {
      return FAILURE;
    }
  }
  return SUCCESS;
}

void CgiSocket::SetSocket(int child_sock, int parent_sock) {
  // 使用しない親プロセス側のソケットを閉じる
  if (close(parent_sock) < 0) {
    // 終了するために例外を投げる
    // 子プロセスなので即終了でOK
    // 課題要件上exit()を使ってはいけないので、例外を投げる
    throw std::runtime_error("Keep Running Error: close in CGI script procces");
  }

  // 子プロセス側のソケットを設定
  // 子プロセスの標準入出力をソケットに変更
  if (dup2(child_sock, STDIN_FILENO) < 0 ||
      dup2(child_sock, STDOUT_FILENO) < 0) {
    // 終了するために例外を投げる
    // 子プロセスなので即終了でOK
    // 課題要件上exit()を使ってはいけないので、例外を投げる
    throw std::runtime_error("Keep Running Error: dup2 in CGI script procces");
  }
}

char **CgiSocket::SetMetaVariables() {
  // http_request_からメタ変数を設定
  std::vector<std::string> meta_variables;

  // GATEWAY_INTERFACE
  meta_variables.push_back("GATEWAY_INTERFACE=CGI/1.1");
  // TODO: 他のメタ変数はこれから追加

  // メタ変数を構築したvectorをchar**に変換
  char **envp = new char *[meta_variables.size() + 1];
  for (size_t i = 0; i < meta_variables.size(); i++) {
    envp[i] = new char[meta_variables[i].size() + 1];
    std::strcpy(envp[i], meta_variables[i].c_str());
  }

  envp[meta_variables.size()] = NULL;

  return envp;
}

char **CgiSocket::SetArgv() {
  // http_request_からargvを設定
  std::vector<std::string> argv;

  // 実行ファイルのパス
  argv.push_back(this->http_request_.GetContext().resource_path.server_path.c_str());

  // TODO:
  // クエリ文字列に'='があるか判定し、なければ'+'で区切ってコマンドラインに追加

  // argvを構築したvectorをchar**に変換
  char **argvp = new char *[argv.size() + 1];
  for (size_t i = 0; i < argv.size(); i++) {
    argvp[i] = new char[argv[i].size() + 1];
    std::strcpy(argvp[i], argv[i].c_str());
  }

  argvp[argv.size()] = NULL;

  return argvp;
}

Response CgiSocket::ParseCgiResponse() {
  Response http_response;
  ResponseMessage response_message;

  // シナリオ 1:500 Internal Server Error
  response_message.status_code_ = 500;
  response_message.status_message_ = "Internal Server Error";

  // シナリオ 2:

  // シナリオ 3:

  http_response.SetResponseMessage(response_message);
  return http_response;
}

// RFC3875 7.2
// カレントワーキングディレクトリをCGIスクリプトのディレクトリに変更する
void CgiSocket::SetCurrentDir(const std::string &cgi_path) {
  // cgi_pathからディレクトリを取得
  std::string dir_path = cgi_path.substr(0, cgi_path.find_last_of('/'));

  // chdir()でディレクトリを変更
  if (chdir(dir_path.c_str()) < 0) {
    // syscall エラーなので、プロセスを終了するために例外を投げる
    // 子プロセスなので即終了でOK
    // 課題要件上exit()を使ってはいけないので、例外を投げる
    throw std::runtime_error("Keep Running Error: chdir in CGI script procces");
  }
}

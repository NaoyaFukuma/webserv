#include "CgiSocket.hpp"
#include "CgiResponseParser.hpp"
#include "Epoll.hpp"
#include "Http.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Socket.hpp"
#include "define.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <wait.h>

/* ConnScoketクラスと同じアウトラインを持つ
  CgiSocket(ConnSocket *conn_socket);
  ~CgiSocket();
  CgiSocket *CreatCgiProcess();
  int OnWritable(Epoll *epoll);
  int OnReadable(Epoll *epoll);
  int ProcessSocket(Epoll *epoll, void *data);
*/

// CGI実行を要求したHTTPリクエストとクライアントの登録、そのクライアントのconfigを登録しておく
CgiSocket::CgiSocket(ConnSocket *conn_socket, Request &http_request)
    : ASocket(conn_socket->GetConfVec()), conn_socket_(conn_socket),
      fin_http_response_(false) {
  this->send_buffer_.AddString(http_request.GetRequestMessage().body);
  this->cgi_request_.context_ = http_request.GetContext();
  this->cgi_request_.context_.is_cgi = false;
  this->cgi_request_.message_ = http_request.GetRequestMessage();
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

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int CgiSocket::OnReadable(Epoll *epoll) {
  // UNIXドメインソケットからCGIスクリプトの出力を受け取る
  int recv_result = recv_buffer_.ReadSocket(this->GetFd());
  // this->last_event_.in_time = time(NULL); //
  // タイムアウトの起算点の更新はせずにCGIスクリプト起動からの経過時間で固定する

  // 0 ならCGIレスポンスを受信しバッファにためて、あえてまだParseはしない。
  // -1 はfinパケットを受信したことを意味し、Parseに入る
  if (recv_result == 0) {
    return SUCCESS;
  } else {
    // finパケットを受信し、CGIスクリプトが終了したのでCGIレスポンスをParseし、
    // HTTPレスポンスを作成しdequeに追加して、EpollにEPOLLOUTでADDしHTTPクライアントへのレスポンス送信に備える
    CgiResponseParser cgi_res_parser(*this);
    cgi_res_parser.ParseCgiResponse();

    if (cgi_res_parser
            .IsRedirectCgi()) { // ローカルリダイレクト先がさらにCGIスクリプト
      Request new_http_request = cgi_res_parser.GetHttpRequestForCgi();
      CgiSocket *new_cgi_socket =
          new CgiSocket(this->conn_socket_, new_http_request);

      ASocket *cgi_socket = new_cgi_socket->CreatCgiProcess();
      if (cgi_socket == NULL) {
        delete new_cgi_socket;
        // 500 Internal Server Errorをhttp responseに追加する
      }
      uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLET;
      epoll->Add(new_cgi_socket, event_mask);
      this->fin_http_response_ =
          true; // http_responseを作成していないが、リダイレクト先で作成するのでtrue
    } else {    // http responseを作成できる
      this->conn_socket_->PushResponse(cgi_res_parser.GetHttpResponse());
      uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
      epoll->Mod(this->conn_socket_->GetFd(), event_mask);
      this->fin_http_response_ = true;
      // http_response作成完了。trueにしておかないと500 Internal Server
      // Errorを追加
    }
  }
  return FAILURE;
  // FAILUREを返せば、子プロセスのkill()とwaitpid()が、CgiSocketのデストラクタで行われる
  // UNIXドメインのFDのclose()が、基底クラスのASocketのデストラクタで行われる
  // OnWritable()の呼び出し元で、さらにFAILUREを返し、EPOLLの登録も解除される
}

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int CgiSocket::OnWritable(Epoll *epoll) {
  // this->last_event_.out_time = time(NULL); //
  // タイムアウトの起算点の更新はせずにCGIスクリプト起動からの経過時間で固定する
  // send_buffer_の内容をUNIXソケットを通じてCGIスクリプトに書き込む
  int send_result;
  if (send_buffer_.GetBuffSize()) {
    send_result = send_buffer_.SendSocket(this->GetFd());
  } else {
    send_result = 1; // 送信完了扱いとする
  }

  switch (send_result) {

  case 0: { // 送信未完了
    // EPOLLOUTのイベント登録を維持しつつ、次回のEPOLLOUTを発火を待つ
    last_event_.out_time = time(NULL); // 現在時間に更新
    break;
  }
  case 1: { // 送信完了
    // EPOLLOUTのイベント登録を解除し、EPOLLINのイベント登録を行う
    uint32_t event_mask = EPOLLIN | EPOLLET;
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

  if (event_mask & EPOLLOUT) {
    std::cout << "FD: " << this->GetFd();
    std::cout << " CgiSocket::ProcessSocket() EPOLLOUT" << std::endl;
    // send_buffer_からの送信。HTTPリクエストにエンティティボディが無い場合はsend_buffer_には何も入っていない。ただし、一度send()のステップを踏んで、送信完了と合流する
    if (OnWritable(epoll) == FAILURE) {
      if (!this->fin_http_response_) {
        // HTTPクライアントへのレスポンスを生成していないのにCGIが終了したので、500
        // Internal Server Errorを追加する
        Response http_response;
        http_response.SetResponseStatus(500);
        http_response.SetVersion(Http::HTTP11);
        http_response.SetHeader("Content-Length",
                                std::vector<std::string>(1, "0"));
        http_response.SetHeader("Connection",
                                std::vector<std::string>(1, "close"));
        this->conn_socket_->PushResponse(http_response);
        uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
        epoll->Add(this->conn_socket_, event_mask);
      }
      return FAILURE;
    }
  }

  if (event_mask & EPOLLIN) {
    std::cout << "FD: " << this->GetFd();
    std::cout << " CgiSocket::ProcessSocket() EPOLLIN" << std::endl;
    // 受信(Todo: OnReadable(0))
    if (OnReadable(epoll) == FAILURE) {
      if (!this->fin_http_response_) {
        // HTTPクライアントへのレスポンスを生成していないのにCGIが終了したので、500
        // Internal Server Errorを追加する
        Response http_response;
        http_response.SetResponseStatus(500);
        http_response.SetVersion(Http::HTTP11);
        http_response.SetHeader("Content-Length",
                                std::vector<std::string>(1, "0"));
        http_response.SetHeader("Connection",
                                std::vector<std::string>(1, "close"));
        this->conn_socket_->PushResponse(http_response);
        uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
        epoll->Add(this->conn_socket_, event_mask);
      }
      return FAILURE;
    }
  }


  // if (event_mask & EPOLLERR || event_mask & EPOLLHUP) {
  if (event_mask & EPOLLHUP) {
    std::cout << "FD: " << this->GetFd();
    std::cout << " CgiSocket::ProcessSocket() EPOLLHUP" << std::endl;
    // エラーイベント keep runningエラーを出力はしつつ、クライアントには500
    // Internal Server Errorを返す
    std::cerr << "Keep Running Error: EPOLLHUP" << std::endl;

    // HTTP 500 Internal Server Error を、クライアントのresponse_dequeに追加する
    if (!this->fin_http_response_) {
      // HTTPクライアントへのレスポンスを生成していないのにCGIが終了したので、500
      // Internal Server Errorを追加する
      Response http_response;
      http_response.SetResponseStatus(500);
      http_response.SetVersion(Http::HTTP11);
      http_response.SetHeader("Content-Length",
                              std::vector<std::string>(1, "0"));
      http_response.SetHeader("Connection",
                              std::vector<std::string>(1, "close"));
      this->conn_socket_->PushResponse(http_response);
      uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
      epoll->Add(this->conn_socket_, event_mask);
    }
    return FAILURE;
  }
  return SUCCESS;
}

Context &CgiSocket::GetContext() { return this->cgi_request_.context_; }

RequestMessage &CgiSocket::GetRequestMessage() {
  return this->cgi_request_.message_;
}

#include "CgiSocket.hpp"
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

  // タイムアウトの起算点を設定
  this->last_event_.out_time = time(NULL);
  this->last_event_.in_time = time(NULL);

  // CgiSocketを返り値にしてEPOLLにEPOLLOUTでADDしてもらう
  return this;
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
  // REMOTE_ADDR
  meta_variables.push_back("REMOTE_ADDR=" +
                           this->conn_socket_->GetIpPort().first);
  // REQUEST_METHOD
  meta_variables.push_back(
      "REQUEST_METHOD=" +
      this->http_request_.GetRequestMessage().request_line.method);
  // SCRIPT_NAME
  meta_variables.push_back(
      "SCRIPT_NAME=" +
      this->http_request_.GetContext().resource_path.server_path);
  // SERVER_NAME
  meta_variables.push_back(
      "SERVER_NAME=" + this->http_request_.GetContext().resource_path.uri.path);
  // SERVER_PORT
  std::stringstream ss;
  ss << ntohl(this->conn_socket_->GetConf()[0].listen_.sin_port);
  meta_variables.push_back("SERVER_PORT=" + ss.str());

  // SERVER_PROTOCOL
  enum Http::Version ver =
      this->http_request_.GetRequestMessage().request_line.version;
  std::string version;
  switch (ver) {
  case Http::HTTP09:
    version = "HTTP/0.9";
    break;
  case Http::HTTP10:
    version = "HTTP/1.0";
    break;
  case Http::HTTP11:
    version = "HTTP/1.1";
    break;
  default:
    break;
  }
  meta_variables.push_back("SERVER_PROTOCOL=" + version);
  // SERVER_SOFTWARE
  meta_variables.push_back("SERVER_SOFTWARE=webserv/0.1");
  // CONTENT_LENGTH
  // bodyのサイズをstringに変換して、メタ変数に追加
  ss.str("");
  ss.clear();
  ss << this->http_request_.GetRequestMessage().body.size();
  meta_variables.push_back("CONTENT_LENGTH=" + ss.str());
  // PATH_INFO
  meta_variables.push_back(
      "PATH_INFO=" + this->http_request_.GetContext().resource_path.path_info);
  // QUERY_STRING
  std::string query_string =
      this->http_request_.GetContext().resource_path.uri.query;
  // '&'が含まれている場合にのみ環境変数に追加、含まれない場合は"QUERY_STRING="と設定、別の関数でコマンドライン引数とする
  if (query_string.find('&') != std::string::npos) {
    // '%'があったので、%デコードを行い環境変数に設定
    query_string = Http::DeHexify(query_string);
    meta_variables.push_back("QUERY_STRING=" + query_string);
  } else { // '&'が含まれていないので、"QUERY_STRING="と設定
    meta_variables.push_back("QUERY_STRING=");
  }
  // REMOTE_HOST これは対応しない RFC3875 MAY
  // AUTH_TYPE これは対応しない RFC3875 MAY
  // CONTENT_TYPE
  if (this->http_request_.GetRequestMessage().header.find("Content-Type") !=
      this->http_request_.GetRequestMessage().header.end()) {
    meta_variables.push_back(
        "CONTENT_TYPE=" +
        this->http_request_.GetRequestMessage().header["Content-Type"][0]);
  } else {
    meta_variables.push_back("CONTENT_TYPE=");
  }

  // PATH_TRANSLATED
  std::string path = this->http_request_.GetContext().resource_path.server_path;
  path = path.substr(0, path.find_last_of('/'));
  path += this->http_request_.GetContext().resource_path.path_info;
  meta_variables.push_back("PATH_TRANSLATED=" + path);
  // REMOTE_IDENT これは対応しない RFC3875 MAY
  // REMOTE_USER これは対応しない RFC3875 MAY
  // SCRIPT_FILENAME
  meta_variables.push_back("SCRIPT_FILENAME=" +
                           this->http_request_.GetContext().server_name);

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
  argv.push_back(
      this->http_request_.GetContext().resource_path.server_path.c_str());

  std::string query_string =
      this->http_request_.GetContext().resource_path.uri.query;
  // '&'が含まれている場合にのみ環境変数に追加、含まれない場合は"QUERY_STRING="と設定、この関数でコマンドライン引数とする
  if (query_string.find('&') == std::string::npos) {
    // %が無いので、argvに追加
    // '+'をdemiliterとしてsubstr()し、%デコードを施しargvにpush_back
    std::string delimiter = "+";
    size_t pos = 0;
    std::string token;
    while ((pos = query_string.find(delimiter)) != std::string::npos) {
      token = query_string.substr(0, pos);
      token = Http::DeHexify(token);
      argv.push_back(token);
      query_string.erase(0, pos + delimiter.length());
    }
  }

  // argvを構築したvectorをchar**に変換
  char **argvp = new char *[argv.size() + 1];
  for (size_t i = 0; i < argv.size(); i++) {
    argvp[i] = new char[argv[i].size() + 1];
    std::strcpy(argvp[i], argv[i].c_str());
  }

  argvp[argv.size()] = NULL;

  return argvp;
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

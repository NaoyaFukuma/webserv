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

/*
CgiScriptのプロセスを生成し、EPOLL登録用のポインタを返す
エラー時にはNULLを返す
*/
CgiSocket *CgiSocket::CreatCgiProcess() {
  if (this->CreatUnixDomainSocketPair() == FAILURE) {
    return NULL;
  }
  if (this->ForkWrapper() == FAILURE) {
    return NULL;
  }
  if (this->pid_ == 0) {
    ExeCgiScript();
  }
  if (SetParentProcessSocket() == FAILURE) {
    return NULL;
  }
  this->last_event_.out_time = time(NULL);
  this->last_event_.in_time = time(NULL);
  return this;
}

int CgiSocket::CreatUnixDomainSocketPair() {
  int cgi_socket[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, cgi_socket) < 0) {
    std::cerr << "Keep Running Error: socketpair" << std::endl;
    return FAILURE;
  }
  parent_sock_ = cgi_socket[0];
  child_sock_ = cgi_socket[1];
  return SUCCESS;
}

void CgiSocket::ExeCgiScript() {
  this->SetChildProcessSocket();
  char **env = this->SetChildProcessMetaVariables();
  char **argv = this->SetChildProcessArgv();
  this->SetChildProcessCurrentDir(this->GetContext().resource_path.server_path);
  const char *cgi_file_path =
      this->GetContext().resource_path.server_path.c_str();
  if (execve(cgi_file_path, argv, env) < 0) {
    // 例外を投げ子プロセス終了 exit()使用不可
    throw std::runtime_error(
        "Keep Running Error: execve in CGI script procces");
  }
}

void CgiSocket::SetChildProcessSocket() {
  if (close(this->parent_sock_) < 0) {
    // 例外を投げ子プロセス終了 exit()使用不可
    throw std::runtime_error("Keep Running Error: close in CGI script procces");
  }
  // 子プロセスで使用するソケットのFDを標準入出力に設定
  if (dup2(this->child_sock_, STDIN_FILENO) < 0 ||
      dup2(this->child_sock_, STDOUT_FILENO) < 0) {
    throw std::runtime_error("Keep Running Error: dup2 in CGI script procces");
  }
  if (close(this->child_sock_) < 0) {
    throw std::runtime_error("Keep Running Error: close in CGI script procces");
  }
}

char **CgiSocket::SetChildProcessMetaVariables() {
  std::vector<std::string> meta;

  meta.push_back("GATEWAY_INTERFACE=CGI/1.1");
  meta.push_back("REMOTE_ADDR=" + this->conn_socket_->GetIpPort().first);
  meta.push_back("REQUEST_METHOD=" +
                 this->GetRequestMessage().request_line.method);
  meta.push_back("SCRIPT_NAME=" + this->GetContext().resource_path.uri.path);
  meta.push_back("SERVER_NAME=" + this->GetContext().server_name);

  std::stringstream ss;
  ss << ntohl(this->conn_socket_->GetConfVec()[0].listen_.sin_port);
  meta.push_back("SERVER_PORT=" + ss.str());
  ss.str("");
  ss.clear();

  enum Http::Version ver = this->GetRequestMessage().request_line.version;
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
  meta.push_back("SERVER_PROTOCOL=" + version);
  meta.push_back("SERVER_SOFTWARE=webserv/0.1");

  ss << this->GetRequestMessage().body.size();
  meta.push_back("CONTENT_LENGTH=" + ss.str());
  meta.push_back("PATH_INFO=" + this->GetContext().resource_path.path_info);

  std::string query_string = this->GetContext().resource_path.uri.query;
  // '='が含まれていれば環境変数。含まれていなければコマンドライン引数にする。
  if (query_string.find('=') != std::string::npos) {
    // '='があったので、%デコードを行い環境変数に設定
    query_string = Http::DeHexify(query_string);
    meta.push_back("QUERY_STRING=" + query_string);
  } else { // '='が含まれていないので、"QUERY_STRING="と設定
    meta.push_back("QUERY_STRING=");
  }
  // REMOTE_HOST これは対応しない RFC3875 MAY
  // AUTH_TYPE これは対応しない RFC3875 MAY

  if (this->GetRequestMessage().header.find("Content-Type") !=
      this->GetRequestMessage().header.end()) {
    meta.push_back("CONTENT_TYPE=" +
                   this->GetRequestMessage().header["Content-Type"][0]);
  } else {
    meta.push_back("CONTENT_TYPE=");
  }

  std::string root = this->GetContext().location.root_;
  std::string path_info = this->GetContext().resource_path.path_info;
  // rootの末尾とpath_infoの先頭の'/'の双方を確認し、一つだけ'/'を付与する
  if (root[root.size() - 1] == '/' && path_info[0] == '/') {
    path_info = path_info.substr(1);
  } else if (root[root.size() - 1] != '/' && path_info[0] != '/') {
    root += '/';
  } else {
    // どちらかが'/'であるので、何もしない
  }
  meta.push_back("PATH_TRANSLATED=" + root + path_info);

  // REMOTE_IDENT これは対応しない RFC3875 MAY
  // REMOTE_USER これは対応しない RFC3875 MAY
  meta.push_back("SCRIPT_FILENAME=" +
                 this->GetContext().resource_path.server_path);

  // メタ変数を構築したvectorをchar**に変換
  char **envp = new char *[meta.size() + 1];
  for (size_t i = 0; i < meta.size(); i++) {
    envp[i] = new char[meta[i].size() + 1];
    std::strcpy(envp[i], meta[i].c_str());
  }
  envp[meta.size()] = NULL;
  return envp;
}

char **CgiSocket::SetChildProcessArgv() {
  std::vector<std::string> argv;

  argv.push_back(this->GetContext().resource_path.server_path.c_str());

  std::string query_string = this->GetContext().resource_path.uri.query;
  // '='が含まれていれば環境変数。含まれていなければコマンドライン引数にする。
  if (query_string.find('=') == std::string::npos) {
    // '+'をdemiliterとして分解し、%デコードを施しargvにpush_back
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

  char **argvp = new char *[argv.size() + 1];
  for (size_t i = 0; i < argv.size(); i++) {
    argvp[i] = new char[argv[i].size() + 1];
    std::strcpy(argvp[i], argv[i].c_str());
  }
  argvp[argv.size()] = NULL;
  return argvp;
}

// RFC3875 7.2 カレントワーキングディレクトリをCGIスクリプトのディレクトリに変更する
void CgiSocket::SetChildProcessCurrentDir(const std::string &cgi_path) {
   std::string dir_path = cgi_path.substr(0, cgi_path.find_last_of('/'));
  if (chdir(dir_path.c_str()) < 0) {
    throw std::runtime_error("Keep Running Error: chdir in CGI script procces");
  }
}

int CgiSocket::ForkWrapper() {
  this->pid_ = fork();
  if (this->pid_ < 0) {
    std::cerr << "Keep Running Error: fork" << std::endl;
    if (close(this->parent_sock_) < 0) {
      std::cerr << "Keep Running Error: close" << std::endl;
    }
    if (close(this->child_sock_) < 0) {
      std::cerr << "Keep Running Error: close" << std::endl;
    }
    return FAILURE; // 呼び出し元はHTTPクライアントに500エラーを返す
  }
  return SUCCESS;
}

int CgiSocket::SetParentProcessSocket() {
  if (close(this->child_sock_) < 0) {
    std::cerr << "Keep Running Error: close" << std::endl;
    if (close(this->parent_sock_) < 0) {
      std::cerr << "Keep Running Error: close" << std::endl;
    }
    // 子プロセスを終了させる
    if (kill(this->pid_, SIGTERM) < 0) {
      std::cerr << "Keep Running Error: kill" << std::endl;
    }
    return FAILURE; // HTTPクライアントには500エラーを返してもらう
  }
  // 親プロセス側のソケットFDを基底クラスに設定
  this->SetFd(this->parent_sock_);
  return SUCCESS;
}

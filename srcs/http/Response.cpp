#include "Response.hpp"
#include "Socket.hpp"

Response::Response() { process_status_ = PROCESSING; }

Response::~Response() {}

Response::Response(const Response &src) { *this = src; }

Response &Response::operator=(const Response &src) {
  if (this != &src) {
    process_status_ = src.process_status_;
    message_ = src.message_;
  }
  return *this;
}

std::string Response::GetString() { return "hoge"; }

ProcessStatus Response::GetProcessStatus() const { return process_status_; }

void Response::ProcessRequest(Request &request, ConnSocket *socket,
                              Epoll *epoll) {
  // request.ResolvePath(config);
  if (request.GetContext().is_cgi) {
    // CGI
    CgiSocket cgi_socket(socket, request);
    ASocket *sock = cgi_socket.CreatCgiProcess();
    if (sock == NULL) {
      // エラー処理 クライアントへ500 Internal Server Errorを返す
      return;
    }
    epoll->AddSocket(sock, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
  } else {
    // 静的ファイル
    ProcessStatic(request, socket, epoll);
  }
}

void Response::ProcessStatic(Request &request, ConnSocket *socket,
                             Epoll *epoll) {
  (void)request;
  (void)epoll;
  (void)socket;
}

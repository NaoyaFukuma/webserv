#include "Response.hpp"

Response::Response() { process_status_ = PROCESSING; }

Response::~Response() {}

Response::Response(const Response &src) { *this = src; }

Response &Response::operator=(const Response &src) {
  if (this != &src) {
    process_status_ = src.process_status_;
    status_code_ = src.status_code_;
    status_message_ = src.status_message_;
    header_ = src.header_;
    body_ = src.body_;
  }
  return *this;
}

std::string Response::GetString() { return "hoge"; }

ProcessStatus Response::GetProcessStatus() const { return process_status_; }

void Response::ProcessRequest(Request &request, ConnSocket *socket,
                              Epoll *epoll) {
  // request.ResolvePath(socket->GetConf());
  // Context context = request.GetContext();
  // if (context.location.return_.) {
  //   // returnディレクティブがあった場合、問答無用で返す
  // }
  if (context.is_cgi) {
    // CGI
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
  RequestMessage message = request.GetRequestMessage();
  if (message.request_line.method == "GET") {

  } else if (message.request_line.method == "DELETE") {

  } else {
    // 405 Method Not Allowed
    SetResponseStatus(Http::HttpStatus(405));
  }
}

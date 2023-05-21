#include "Response.hpp"

Response::Response() { process_status_ = PROCESSING; }

Response::~Response() {}

Response::Response(const Response &src) { *this = src; }

Response &Response::operator=(const Response &src) {
  if (this != &src) {
    process_status_ = src.process_status_;
    version_ = src.version_;
    status_code_ = src.status_code_;
    status_message_ = src.status_message_;
    header_ = src.header_;
    body_ = src.body_;
  }
  return *this;
}

std::string Response::GetString() { return "hoge"; }

ProcessStatus Response::GetProcessStatus() const { return process_status_; }

void Response::ProcessRequest(Request &request, ConfVec &config,
                              ConnSocket *socket) {
  // request.ResolvePath(config);
  if (request.GetContext().is_cgi) {
    // CGI
  } else {
    // 静的ファイル
    ProcessStatic(request, config, socket);
  }
}

void Response::ProcessStatic(Request &request, ConfVec &config,
                             ConnSocket *socket) {
  (void)request;
  (void)config;
  (void)socket;
}

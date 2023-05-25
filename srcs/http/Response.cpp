#include "Response.hpp"
#include "Socket.hpp"
#include "CgiSocket.hpp"
#include "Epoll.hpp"
#include <sys/epoll.h>
#include "Epoll.hpp"
#include "Socket.hpp"

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

void Response::SetResponseStatus(Http::HttpStatus status) {
  status_code_ = status.status_code;
  status_message_ = status.message;
}

void Response::SetVersion(Http::Version version) { version_ = version; }

void Response::SetHeader(std::string key, std::string value) {
  if (header_.find(key) != header_.end()) {
    header_[key].push_back(value);
  } else {
    header_[key] = std::vector<std::string>{value};
  }
}

void Response::SetBody(std::string body) {
  SetHeader("Content-Length", std::to_string(body.length()));
  body_ += body;
}

void Response::ProcessRequest(Request &request, ConnSocket *socket,
                              Epoll *epoll) {
  // request.ResolvePath(config);
  if (request.GetContext().is_cgi) {
    // CGI
    CgiSocket *cgi_socket = new CgiSocket(socket, request);
    ASocket *sock = cgi_socket->CreatCgiProcess();
    if (sock == NULL) {
      delete cgi_socket;
      // エラー処理 クライアントへ500 Internal Server Errorを返す
      return;
    }
    uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    epoll->Add(sock, event_mask);
  } else {
    // 静的ファイル
    ProcessStatic(request, socket, epoll);
  }
}

void Response::ProcessStatic(Request &request, ConnSocket *socket,
                             Epoll *epoll) {
  if (request.GetRequestStatus().status_code != -1) {
    SetResponseStatus(request.GetRequestStatus());
    process_status_ = DONE;
    epoll->Mod(socket->GetFd(), EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
    return;
  }
  // Todo: resolvepathはrequest parserの時点で行う
  request.ResolvePath(socket->GetConfVec());
  const Context &context = request.GetContext();
  if (context.location.return_.return_type_ != RETURN_EMPTY) {
    ProcessReturn(request, socket, epoll);
    return;
  }
  if (context.is_cgi) {
    // CGI
  } else {
    // 静的ファイル
    ProcessStatic(request, socket, epoll);
  }
}

void Response::ProcessReturn(Request &request, ConnSocket *socket,
                             Epoll *epoll) {
  const Return &return_ = request.GetContext().location.return_;
  SetResponseStatus(Http::HttpStatus(return_.status_code_));
  switch (return_.return_type_) {
  case RETURN_ONLY_STATUS_CODE:
    break;
  case RETURN_TEXT:
    SetBody(return_.return_text_);
    break;
  case RETURN_URL:
    SetHeader("Location", return_.return_url_);
    break;
  }
  epoll->Mod(socket->GetFd(), EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
  process_status_ = DONE;
}

void Response::ProcessStatic(Request &request, ConnSocket *socket,
                             Epoll *epoll) {
  RequestMessage message = request.GetRequestMessage();
  if (message.request_line.method == "GET") {
    ProcessGET(request, socket, epoll);
  } else if (message.request_line.method == "DELETE") {
    // ProcessDELETE(request, socket, epoll);
  } else {
    // 405 Method Not Allowed
    SetResponseStatus(Http::HttpStatus(405));
  }
  epoll->Mod(socket->GetFd(), EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
  process_status_ = DONE;
}

void Response::ProcessGET(Request &request, ConnSocket *socket, Epoll *epoll) {
  // 指定されたpathがディレクトリだった場合、index, autoindexを解決する
  // indexの解決: location path + index -> 存在しなければautoindexのチェック
  // autoindexの解決:
  if (request.GetContext().resource_path is directory) {
    // processdirectory
    // indexの解決
    if (index in location && IsExist(location.path + index)) {
      ProcessFile(location.path + index);
    } else if (autoindex in location) {
      SetBody(autoindex());
    } else {
      // 404 Not Found
    }
  } else if (IsExist(request.GetContext().resource_path) is file) {
    ProcessFile(request.GetContext().resource_path);
  } else {
    // 404 Not Found
  }
}

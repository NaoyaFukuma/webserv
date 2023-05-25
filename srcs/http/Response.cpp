#include "Response.hpp"
#include "Epoll.hpp"
#include "Socket.hpp"
#include "utils.hpp"

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

std::vector<std::string> Response::GetHeader(const std::string &key) {
  return header_[key];
}

bool Response::HasHeader(const std::string &key) const {
  return header_.find(key) != header_.end();
}

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
  if (request.GetRequestStatus().status_code != -1) {
    SetResponseStatus(request.GetRequestStatus());
    process_status_ = DONE;
    epoll->Mod(socket->GetFd(), EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
    return;
  }
  // Todo: resolvepathはrequest parserの時点で行う
  request.ResolvePath(socket->GetConfig());
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
    ProcessGET(request);
  } else if (message.request_line.method == "DELETE") {
    // ProcessDELETE(request, socket, epoll);
  } else {
    // 405 Method Not Allowed
    SetResponseStatus(Http::HttpStatus(405));
  }
  epoll->Mod(socket->GetFd(), EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
  process_status_ = DONE;
}

void Response::ProcessGET(Request &request) {
  // 指定されたpathがディレクトリだった場合、index, autoindexを解決する
  // indexの解決: location path + index -> 存在しなければautoindexのチェック
  // autoindexの解決:
  Context context = request.GetContext();
  std::string path = context.resource_path.server_path;
  FileType ftype = get_filetype(path);

  if (ftype == FileType::FILE_DIRECTORY) {
    if (!context.location.index_.empty() &&
        get_filetype(path + '/' + context.location.index_) ==
            FileType::FILE_REGULAR) {
      ProcessFile(request, path + '/' + context.location.index_);
    } else if (context.location.autoindex_) {
      // Todo: autoindex
    } else {
      // 404 Not Found
      SetResponseStatus(Http::HttpStatus(404));
    }
  } else if (ftype == FileType::FILE_REGULAR) {
    ProcessFile(request, path);
  } else {
    // 404 Not Found
    SetResponseStatus(Http::HttpStatus(404));
  }
}

void Response::ProcessFile(Request &request, const std::string &path) {
  // Check If headers.
  iIfMod = IfModSince(hInfo, sBuf.st_mtime);
  iIfUnmod = IfUnmodSince(hInfo, sBuf.st_mtime);
  iIfMatch = IfMatch(hInfo, sBuf.st_mtime);
  iIfNone = IfNone(hInfo, sBuf.st_mtime);
  iIfRange = IfRange(hInfo, sBuf.st_mtime);
  iRangeErr = hInfo->FindRanges(sBuf.st_size);

  // Check to make sure any If headers are FALSE.
  // Either not-modified or no etags matched.
  if ((iIfMod == FALSE) || (iIfNone == FALSE)) {
    sClient->Send("HTTP/1.1 304 Not Modified\r\n");
    iRsp = 304;
  }
  // No matching etags or it's been modified.
  else if ((iIfMatch == FALSE) || (iIfUnmod == FALSE)) {
    sClient->Send("HTTP/1.1 412 Precondition Failed\r\n");
    iRsp = 412;
  }
  // Resource matched so send just the bytes requested.
  else if ((iIfRange == TRUE) && (iRangeErr == 0)) {
    sClient->Send("HTTP/1.1 206 Partial Content\r\n");
    iRsp = 206;
  }
  // Resource didn't match, so send the entire entity.
  else {
    sClient->Send("HTTP/1.1 200 OK\r\n");
    iRsp = 200;
  }
}

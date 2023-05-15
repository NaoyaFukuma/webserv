#include "Request.hpp"
#include "utils.hpp"

Request::Request() {
  parse_status_ = INIT;
  error_status_ = -1;
  chunk_status_ = -1;
}

Request::~Request() {}

Request &Request::operator=(const Request &rhs) {
  if (this != &rhs) {
    message_ = rhs.message_;
    parse_status_ = rhs.parse_status_;
    error_status_ = rhs.error_status_;
    chunk_status_ = rhs.chunk_status_;
  }
  return *this;
}

ParseStatus Request::GetParseStatus() const { return parse_status_; }

int Request::GetErrorStatus() const { return error_status_; }

void Request::SetError(int error_status) {
  Clear();
  error_status_ = error_status;
  parse_status_ = ERROR;
}

void Request::Parse(IOBuff &buffer_) {
  std::string line;
  while (parse_status_ != COMPLETE && parse_status_ != ERROR &&
         buffer_.GetUntilCRLF(line)) {
    ParseLine(line);
  }
}

void Request::ParseLine(const std::string &line) {
  switch (parse_status_) {
  case INIT:
    ParseRequestLine(line);
    break;
  case HEADER:
    ParseHeader(line);
    break;
  case BODY:
    ParseBody(line);
    break;
  }
}

void Request::ParseRequestLine(const std::string &line) {
  std::vector<std::string> splited;
  if (ms_split(splited, line, ' ') < 0) {
    SetError(500);
    return;
  } else if (splited.size() != 2 && splited.size() != 3) {
    SetError(400);
    return;
  }
  // HTTP0.9
  if (splited.size() == 2) {
    message_.request_line.method = splited[0];
    if (IsValidMethod(message_.request_line.method) == false) {
      SetError(400);
      return;
    }
    message_.request_line.uri = splited[1];
    message_.request_line.version = HTTP09;
    parse_status_ = COMPLETE;
  }
  // HTTP1.0~
  else {
    message_.request_line.method = splited[0];
    if (IsValidMethod(message_.request_line.method) == false) {
      SetError(400);
      return;
    }
    message_.request_line.uri = splited[1];
    if (splited[2] == "HTTP/1.0") {
      message_.request_line.version = HTTP10;
    } else if (splited[2] == "HTTP/1.1") {
      message_.request_line.version = HTTP11;
    } else {
      SetError(400);
      return;
    }
    parse_status_ = HEADER;
  }
}

void Request::ParseHeader(const std::string &line) {}

void Request::ParseBody(const std::string &line) {}

void Request::Clear() { *this = Request(); }

std::ostream &operator<<(std::ostream &os, const Request &request) {
  RequestMessage m = request.GetRequestMessage();
  os << "RequestLine: " << m.request_line.method << " " << m.request_line.uri
     << " " << m.request_line.version;
}
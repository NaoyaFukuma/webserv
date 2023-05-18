#include "Request.hpp"
#include "Http.hpp"
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

void Request::Parse(SocketBuff &buffer_) {
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

void Request::ResolvePath(Config config) {
  if (HttpUtils::SplitURI(context_.resource_path.uri,
                          message_.request_line.uri) == false) {
    SetError(400);
    return;
  }
  context_.resource_path.query = context_.resource_path.uri.query;

  // hostを決定
  std::string host;
  if (!context_.resource_path.uri.host.empty()) {
    host = context_.resource_path.uri.host;
  }
  if (message_.header.find("Host") != message_.header.end()) {
    host = message_.header["Host"][0];
  }

  // vserverを決定
  std::vector<Vserver> vservers = config.GetServerVec();
  if (host.empty()) {
    context_.vserver = config.GetDefaultServer();
  } else {
    for (std::vector<Vserver>::iterator itv = vservers.begin();
         itv != vservers.end(); itv++) {
      for (std::vector<std::string>::iterator its = itv->server_names_.begin();
           its != itv->server_names_.end(); its++) {
        if (*its == host) {
          context_.vserver = &(*itv);
          break;
        }
      }
      if (context_.vserver != NULL) {
        break;
      }
    }
    if (context_.vserver == NULL) {
      context_.vserver = config.GetDefaultServer();
    }
  }

  // locationを決定
  std::vector<Location> locations = context_.vserver->locations_;
  // locationsのpathをキーにmapに変換
  // context_.resource_path.uri.pathの最後の/までの部分文字列をキーにする
  // "location /"があることを保証するので、必ずキーが存在する
}

std::ostream &operator<<(std::ostream &os, const Request &request) {
  RequestMessage m = request.GetRequestMessage();
  os << "RequestLine: " << m.request_line.method << " " << m.request_line.uri
     << " " << m.request_line.version;
}
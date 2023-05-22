#include "Request.hpp"
#include "utils.hpp"

Request::Request() {
  parse_status_ = INIT;
  chunk_status_ = -1;
}

Request::~Request() {}

// これがないとcompile時に怒られたので追加
Request::Request(const Request &src) { *this = src; }

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

Http::HttpError Request::GetErrorStatus() const { return error_status_; }

Context Request::GetContext() const { return context_; }

void Request::SetError(int error_status) {
  error_status_.status_code = error_status;
  // error_status_.status_message = Http::GetStatusMessage(error_status);
  parse_status_ = ERROR;
}

void Request::Parse(SocketBuff &buffer_) {
  // std::string line;
  // while (parse_status_ != COMPLETE && parse_status_ != ERROR &&
  //        buffer_.GetUntilCRLF(line)) {
  //   ParseLine(line);
  // }
  (void)buffer_;
  message_.request_line.method = "GET";
  message_.request_line.uri = "/index.html";
  message_.request_line.version = Http::HTTP09;
  parse_status_ = COMPLETE;
}

// void Request::ParseLine(const std::string &line) {
//   switch (parse_status_) {
//   case INIT:
//     ParseRequestLine(line);
//     break;
//   case HEADER:
//     ParseHeader(line);
//     break;
//   case BODY:
//     ParseBody(line);
//     break;
//   }
// }

// void Request::ParseRequestLine(const std::string &line) {
//   std::vector<std::string> splited;
//   if (ms_split(splited, line, ' ') < 0) {
//     SetError(500);
//     return;
//   } else if (splited.size() != 2 && splited.size() != 3) {
//     SetError(400);
//     return;
//   }
//   // HTTP0.9
//   if (splited.size() == 2) {
//     message_.request_line.method = splited[0];
//     if (IsValidMethod(message_.request_line.method) == false) {
//       SetError(400);
//       return;
//     }
//     message_.request_line.uri = splited[1];
//     message_.request_line.version = HTTP09;
//     parse_status_ = COMPLETE;
//   }
//   // HTTP1.0~
//   else {
//     message_.request_line.method = splited[0];
//     if (IsValidMethod(message_.request_line.method) == false) {
//       SetError(400);
//       return;
//     }
//     message_.request_line.uri = splited[1];
//     if (splited[2] == "HTTP/1.0") {
//       message_.request_line.version = HTTP10;
//     } else if (splited[2] == "HTTP/1.1") {
//       message_.request_line.version = HTTP11;
//     } else {
//       SetError(400);
//       return;
//     }
//     parse_status_ = HEADER;
//   }
// }

// void Request::ParseHeader(const std::string &line) {}

// void Request::ParseBody(const std::string &line) {}

// void Request::Clear() { *this = Request(); }

void Request::ResolvePath(const Config &config) {
  std::string src_uri = Http::DeHexify(message_.request_line.uri);
  if (Http::SplitURI(context_.resource_path.uri, src_uri) == false) {
    SetError(400);
    return;
  }

  // hostを決定
  std::string host = ResolveHost();

  // vserverを決定
  context_.vserver = ResolveVserver(config, host);

  // locationを決定
  context_.location =
      ResolveLocation(*context_.vserver, context_.resource_path.uri.path);
}

std::string Request::ResolveHost() {
  std::string host = "";
  Http::URI uri = context_.resource_path.uri;

  if (!uri.host.empty()) {
    host = uri.host;
  }
  if (message_.header.find("Host") != message_.header.end()) {
    host = message_.header["Host"][0];
  }
  return host;
}

Vserver *Request::ResolveVserver(const Config &config,
                                 const std::string &host) {
  Vserver *vserver = NULL;
  ConfVec vservers = config.GetServerVec();

  // vservers[0]: default vserver
  if (host.empty()) {
    vserver = &vservers[0];
  } else {
    for (ConfVec::iterator itv = vservers.begin(); itv != vservers.end();
         itv++) {
      for (std::vector<std::string>::iterator its = itv->server_names_.begin();
           its != itv->server_names_.end(); its++) {
        if (*its == host) {
          vserver = &(*itv);
          break;
        }
      }
      if (vserver != NULL) {
        break;
      }
    }
    if (vserver == NULL) {
      vserver = &vservers[0];
    }
  }
  return vserver;
}

Location *Request::ResolveLocation(Vserver &vserver, const std::string &path) {
  std::vector<Location> &locations = vserver.locations_;

  // locationsのpathをキーにmapに変換
  std::map<std::string, Location> location_map;
  for (std::vector<Location>::const_iterator it = locations.begin();
       it != locations.end(); it++) {
    location_map[it->path_] = *it;
  }

  // context_.resource_path.uri.pathの最後の/までの部分文字列をキーにする
  std::vector<std::string> keys;
  for (std::string::const_iterator it = path.begin(); it != path.end(); it++) {
    if (*it == '/') {
      keys.push_back(path.substr(0, it - path.begin() + 1));
    }
  }
  // path != "" (uriの分解時に空文字列の場合は'/'としている)
  if (path[path.size() - 1] != '/') {
    keys.push_back(path);
  }

  // "location /"があることを保証するので、必ずキーが存在する
  for (std::vector<std::string>::iterator it = keys.begin(); it != keys.end();
       it++) {
    if (location_map.find(*it) != location_map.end()) {
      return &location_map[*it];
    }
  }
  // ここのreturnは実行されないはず
  // locations[0] = "location /"
  return &locations[0];
}

// std::ostream &operator<<(std::ostream &os, const Request &request) {
//   RequestMessage m = request.GetRequestMessage();
//   os << "RequestLine: " << m.request_line.method << " " << m.request_line.uri
//      << " " << m.request_line.version;
// }
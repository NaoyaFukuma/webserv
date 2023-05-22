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

Header Request::GetHeaderMap() const { return message_.header; }

// void Request::SetError(int error_status) {
//   Clear();
//   error_status_ = error_status;
//   parse_status_ = ERROR;
// }

void Request::Parse(SocketBuff &buffer_) {
  std::string line;
  while (parse_status_ != COMPLETE && parse_status_ != ERROR &&
         buffer_.GetUntilCRLF(line)) {
    ParseLine(line);
  }
  (void) buffer_;
  message_.request_line.method = "GET";
  message_.request_line.uri = "/index.html";
  message_.request_line.version = Http::HTTP09;
  parse_status_ = COMPLETE;
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
    case COMPLETE:
      break;
    case ERROR:
      // TODO: ここでエラー処理
      break;
  }
}

void Request::ParseRequestLine(const std::string &line) {
  std::vector<std::string> splited;
  // TODO: split要変更
  if (ws_split(splited, line, ' ') < 0) {
//     SetError(500);
    return;
  } else if (splited.size() != 2 && splited.size() != 3) {
//     SetError(400);
    return;
  }
  // HTTP0.9
  if (splited.size() == 2) {
    message_.request_line.method = splited[0];
//     if (IsValidMethod(message_.request_line.method) == false) {
////       SetError(400);
//       return;
//     }
    message_.request_line.uri = splited[1];
    message_.request_line.version = Http::HTTP09;
    parse_status_ = COMPLETE;
  }
    // HTTP1.0~
  else {
    message_.request_line.method = splited[0];
//     if (IsValidMethod(message_.request_line.method) == false) {
////       SetError(400);
//       return;
//     }
    message_.request_line.uri = splited[1];
    if (splited[2] == "HTTP/1.0") {
      message_.request_line.version = Http::HTTP10;
    } else if (splited[2] == "HTTP/1.1") {
      message_.request_line.version = Http::HTTP11;
    } else {
//       SetError(400);
      return;
    }
    parse_status_ = HEADER;
  }
}

void Request::ParseHeader(const std::string &line) {
  // 空行の場合BODYに移行
  if (line == "\r\n") {
    parse_status_ = BODY;
    return;
  }

  // findで':'を探す
  std::string::size_type pos = line.find(':');
  if (pos == std::string::npos) {
    // TODO: エラー処理
    // ':'がない
    return;
  }
  // ':'の前後で分割
  std::string key = line.substr(0, pos);
  // key が正しい文字列がエラー処理?

  std::vector<std::string> header_values;
  // pos+1以降の文字列を','ごとに分割
  // pos以降のスペースをスキップする // TODO: スキップすべきスペースは？
  pos = MovePos(line, pos + 1, " \t");
  if (ws_split(header_values, line.substr(pos), ',') < 0) {
    // TODO: エラー処理
    return;
  }
  // Headerのkeyとvalueを格納
  message_.header[key] = header_values;
}

void Request::ParseBody(const std::string &line) {
  (void) line;
}

// void Request::Clear() { *this = Request(); }

// void Request::ResolvePath(Config config) {
//   std::string src_uri = Http::DeHexify(message_.request_line.uri);
//   if (Http::SplitURI(context_.resource_path.uri, src_uri) == false) {
//     SetError(400);
//     return;
//   }

//   // hostを決定
//   std::string host;
//   if (!context_.resource_path.uri.host.empty()) {
//     host = context_.resource_path.uri.host;
//   }
//   if (message_.header.find("Host") != message_.header.end()) {
//     host = message_.header["Host"][0];
//   }

//   // vserverを決定
//   std::vector<Vserver> vservers = config.GetServerVec();
//   if (host.empty()) {
//     context_.vserver = &vservers[0];
//   } else {
//     for (std::vector<Vserver>::iterator itv = vservers.begin();
//          itv != vservers.end(); itv++) {
//       for (std::vector<std::string>::iterator its =
//       itv->server_names_.begin();
//            its != itv->server_names_.end(); its++) {
//         if (*its == host) {
//           context_.vserver = &(*itv);
//           break;
//         }
//       }
//       if (context_.vserver != NULL) {
//         break;
//       }
//     }
//     if (context_.vserver == NULL) {
//       context_.vserver = &vservers[0];
//     }
//   }

//   // locationを決定
//   std::vector<Location> locations = context_.vserver->locations_;
//   // locationsのpathをキーにmapに変換
//   std::map<std::string, Location> location_map;
//   for (std::vector<Location>::iterator it = locations.begin();
//        it != locations.end(); it++) {
//     location_map[it->path_] = *it;
//   }
//   // context_.resource_path.uri.pathの最後の/までの部分文字列をキーにする
//   std::string path = context_.resource_path.uri.path;
//   std::vector<std::string> keys;
//   for (std::string::iterator it = path.begin(); it != path.end(); it++) {
//     if (*it == '/') {
//       keys.push_back(path.substr(0, it - path.begin() + 1));
//     }
//   }

//   // "location /"があることを保証するので、必ずキーが存在する
// }

// std::ostream &operator<<(std::ostream &os, const Request &request) {
//   RequestMessage m = request.GetRequestMessage();
//   os << "RequestLine: " << m.request_line.method << " " << m.request_line.uri
//      << " " << m.request_line.version;
// }

std::string::size_type
Request::MovePos(const std::string &line, std::string::size_type start, const std::string &delim) {
  std::string::size_type pos = start;
  while (pos < line.size() && delim.find(line[pos]) != std::string::npos) {
    pos++;
  }
  return pos;
}
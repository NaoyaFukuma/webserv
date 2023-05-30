#include "Request.hpp"
#include "utils.hpp"
#include <cerrno>
#include <deque>
#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>

Request::Request() {
  parse_status_ = INIT;
  chunk_status_ = -1;
  content_length_ = -1;
}

Request::~Request() {}

// これがないとcompile時に怒られたので追加
Request::Request(const Request &src) { *this = src; }

Request &Request::operator=(const Request &rhs) {
  if (this != &rhs) {
    message_ = rhs.message_;
    parse_status_ = rhs.parse_status_;
    http_status_ = rhs.http_status_;
    chunk_status_ = rhs.chunk_status_;
  }
  return *this;
}

ParseStatus Request::GetParseStatus() const { return parse_status_; }

Http::HttpStatus Request::GetRequestStatus() const { return http_status_; }

void Request::SetRequestStatus(Http::HttpStatus status) {
  http_status_ = status;
}

RequestMessage Request::GetRequestMessage() const { return message_; }

Context Request::GetContext() const { return context_; }

Header Request::GetHeaderMap() const { return message_.header; }

std::vector<std::string> Request::GetHeader(const std::string &key) {
  return message_.header[key];
}

bool Request::HasHeader(const std::string &key) const {
  return message_.header.find(key) != message_.header.end();
}

void Request::Parse(SocketBuff &buffer_) {
  std::string line;
  while (parse_status_ != COMPLETE && parse_status_ != ERROR &&
         buffer_.GetUntilCRLF(line)) {
    ParseLine(line);
  }
  (void)buffer_;
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
  SplitRequestLine(splited, line);
  // TODO: リクエストが有効かどうかのチェック足す
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

// void Request::ParseHeader(const std::string &line) {}

// void Request::ParseBody(const std::string &line) {}

// void Request::Clear() { *this = Request(); }

void Request::ParseHeader(const std::string &line) {
  // 空行の場合BODYに移行
  if (line == "\r\n") {
    parse_status_ = BODY;
    return;
  }

  std::string::size_type pos = line.find(':');
  if (pos == std::string::npos) {
    // TODO: エラー処理
    // ':'がない
    return;
  }
  // ':'の前後で分割
  std::string key = line.substr(0, pos);
  pos++;
  // key が正しい文字列がエラー処理?

  std::vector<std::string> header_values;
  // pos+1以降の文字列を','や' 'ごとに分割
  // pos以降のスペースをスキップする // TODO: スキップすべきスペースは？
  while (pos < line.size()) {
    pos = MovePos(line, pos, " \t");
    header_values.push_back(GetWord(line, pos));
  }
  // Headerのkeyとvalueを格納
  message_.header[key] = header_values;
}

void Request::ParseBody(const std::string &line) {
  if (!JudgeBodyType()) {
    // error
  }
  (void)line;
}

bool Request::JudgeBodyType() {
  // headerにContent-LengthかTransfer-Encodingがあるかを調べる
  Header::iterator it_transfer_encoding =
      message_.header.find("Transfer-Encoding");
  Header::iterator it_content_length = message_.header.find("Content-Length");

  if (it_transfer_encoding != message_.header.end()) {
    std::vector<std::string> values = it_transfer_encoding->second;
    // values を全部探索
    for (std::vector<std::string>::const_iterator it = values.begin();
         it != values.end(); ++it) {
      if (*it == "chunked") {
        chunk_status_ = true;
        return true;
      }
    }
  }
  if (it_content_length != message_.header.end()) {
    std::vector<std::string> values = it_content_length->second;
    // 複数の指定があったらエラー
    // 負の数、strtollのエラーもエラー
    errno = 0;
    long long content_length = std::strtoll(values[0].c_str(), NULL, 10);
    if (values.size() != 1 || errno == ERANGE || content_length < 0) {
      // error
      return false;
    } else {
      content_length_ = content_length;
      return true;
    }
  }
  return false;
}

std::string Request::GetWord(const std::string &line,
                             std::string::size_type &pos) {
  std::string word;
  while (pos < line.size() && !std::isspace(line[pos]) && line[pos] != ',') {
    word += line[pos++];
  }
  pos++;
  return word;
}

// std::ostream &operator<<(std::ostream &os, const Request &request) {
//   RequestMessage m = request.GetRequestMessage();
//   os << "RequestLine: " << m.request_line.method << " " <<
//   m.request_line.uri
//      << " " << m.request_line.version;
// }

bool Request::SplitRequestLine(std::vector<std::string> &splited,
                               const std::string &line) {
  // 空白文字で分割
  // 空白は1文字まで
  std::string::size_type pos = 0;
  while (pos < line.size()) {
    if (std::isspace(line[pos])) {
      return false;
    }
    splited.push_back(GetWord(line, pos));
  }
  if (splited.size() != 2 && splited.size() != 3) {
    return false;
  }
  return true;
}

std::string::size_type Request::MovePos(const std::string &line,
                                        std::string::size_type start,
                                        const std::string &delim) {
  std::string::size_type pos = start;
  while (pos < line.size() && delim.find(line[pos]) != std::string::npos) {
    pos++;
  }
  return pos;
}

void Request::ResolvePath(const ConfVec &vservers) {
  std::string src_uri = message_.request_line.uri;
  if (Http::SplitURI(context_.resource_path.uri, src_uri) == false) {
    // SetError(400);
    return;
  }
  // hostを決定
  std::string host = ResolveHost();

  // vserverを決定
  ResolveVserver(vservers, host);

  // locationを決定
  ResolveLocation();

  Http::DeHexify(context_.resource_path.uri);

  // resource_path, is_cgiを決定
  ResolveResourcePath();
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

void Request::ResolveVserver(const ConfVec &vservers, const std::string &host) {
  // vservers[0]: default vserver
  context_.vserver = vservers[0];
  context_.server_name = vservers[0].server_names_[0];
  if (host.empty()) {
    return;
  } else {
    for (ConfVec::const_iterator itv = vservers.begin(); itv != vservers.end();
         itv++) {
      for (std::vector<std::string>::const_iterator its =
               itv->server_names_.begin();
           its != itv->server_names_.end(); its++) {
        if (*its == host) {
          context_.vserver = *itv;
          context_.server_name = *its;
          return;
        }
      }
    }
  }
}

void Request::ResolveLocation() {
  std::string &path = context_.resource_path.uri.path;
  std::vector<Location> &locations = context_.vserver.locations_;

  // locationsのpathをキーにmapに変換
  std::map<std::string, Location> location_map;
  for (std::vector<Location>::const_iterator it = locations.begin();
       it != locations.end(); it++) {
    location_map[it->path_] = *it;
  }

  // context_.resource_path.uri.pathの最後の/までの部分文字列をキーにする
  std::deque<std::string> keys;
  for (std::string::const_iterator it = path.begin(); it != path.end(); it++) {
    if (*it == '/') {
      keys.push_front(path.substr(0, it - path.begin() + 1));
    }
  }
  // path != "" (uriの分解時に空文字列の場合は'/'としている)
  if (path[path.size() - 1] != '/') {
    keys.push_back(path);
  }

  // "location /"があることを保証するので、必ずキーが存在する
  for (std::deque<std::string>::iterator it = keys.begin(); it != keys.end();
       it++) {
    if (location_map.find(*it) != location_map.end()) {
      context_.location = location_map[*it];
      return;
    }
  }
}

void Request::ResolveResourcePath() {
  std::string &root = context_.location.root_;
  std::string &path = context_.resource_path.uri.path;

  // pathからlocationを除去して、rootを付与
  std::string concat = root + '/' + path.substr(context_.location.path_.size());

  // cgi_extensionsがない場合、path_infoはなく、concatをserver_pathとする
  if (context_.location.cgi_extensions_.empty()) {
    context_.resource_path.server_path = concat;
    context_.resource_path.path_info = "";
    context_.is_cgi = false;
    return;
  }

  // cgi_extensionsがある場合
  for (std::vector<std::string>::iterator ite =
           context_.location.cgi_extensions_.begin();
       ite != context_.location.cgi_extensions_.end(); ite++) {
    std::string cgi_extension = *ite;

    // concatの'/'ごとにextensionを確認
    for (std::string::iterator its = concat.begin(); its != concat.end();
         its++) {
      if (*its == '/') {
        std::string partial_path = concat.substr(0, its - concat.begin());
        if (ws_exist_cgi_file(partial_path, cgi_extension)) {
          context_.resource_path.server_path =
              concat.substr(0, its - concat.begin());
          context_.resource_path.path_info =
              concat.substr(its - concat.begin());
          context_.is_cgi = true;
          return;
        }
      }
    }
    // concatが/で終わらない場合、追加でチェックが必要
    // concatは '/'を含むので、size() > 0が保証されている
    if (concat[concat.size() - 1] != '/' &&
        ws_exist_cgi_file(concat, cgi_extension)) {
      context_.resource_path.server_path = concat;
      context_.is_cgi = true;
      return;
    }
  }

  context_.resource_path.server_path = concat;
  context_.resource_path.path_info = "";
  context_.is_cgi = false;
  return;
}

// for unit-test
void Request::SetMessage(RequestMessage message) { message_ = message; }
void Request::SetContext(Context context) { context_ = context; }

#include "Request.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <deque>
#include <iostream>

Request::Request() {
  parse_status_ = INIT;
  chunk_status_ = -1;
  body_size_ = -1;
  total_header_size_ = 0;
  is_chunked = false;
}

Request::~Request() {}

// これがないとcompile時に怒られたので追加
Request::Request(const Request &src) { *this = src; }

Request &Request::operator=(const Request &rhs) {
  if (this != &rhs) {
    message_ = rhs.message_;
    parse_status_ = rhs.parse_status_;
    //    http_status_ = rhs.http_status_;
    chunk_status_ = rhs.chunk_status_;
  }
  return *this;
}

RequestMessage Request::GetRequestMessage() const { return message_; }

ParseStatus Request::GetParseStatus() const { return parse_status_; }

Http::HttpStatus Request::GetRequestStatus() const { return http_status_; }

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
         parse_status_ != BODY && buffer_.GetUntilCRLF(line)) {
    ParseLine(line);
  }
  if (parse_status_ == BODY) {
    ParseBody(buffer_);
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
  case COMPLETE:
    break;
  case ERROR:
    SetRequestStatus(400);
    break;
  default:
    break;
  }
}

void Request::ParseRequestLine(const std::string &line) {
  std::vector<std::string> splited;
  // フォーマットのチェック
  // TODO: uri
  if (AssertRequestLine(line) == false) {
    parse_status_ = ERROR;
    return;
  }
  ws_split(splited, line, ' ');

  // TODO: リクエストが有効かどうかのチェック足す
  // HTTP0.9
  message_.request_line.method = splited[0];
  message_.request_line.uri = splited[1];
  if (splited.size() == 2) {
    //     if (IsValidMethod(message_.request_line.method) == false) {
    ////       SetError(400);
    //       return;
    //     }
    message_.request_line.version = Http::HTTP09;
    parse_status_ = COMPLETE;
  }
  // HTTP1.0~
  else {
    //     if (IsValidMethod(message_.request_line.method) == false) {
    ////       SetError(400);
    //       return;
    //     }
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

  if (line.empty()) {
    parse_status_ = BODY;
    return;
  }

  if (!ValidateHeaderSize(line)) {
    parse_status_ = ERROR;

    return;
  }

  std::string::size_type pos = line.find(':');
  // ':'がない
  if (pos == std::string::npos) {
    parse_status_ = ERROR;
    return;
  }
  // ':'の前後で分割
  std::string key = line.substr(0, pos);
  pos++;

  std::vector<std::string> header_values;
  // pos+1以降の文字列を','ごとに分割してスペースを取り除く
  // pos以降のスペースをスキップする // TODO: スキップすべきスペースは？
  SplitHeaderValues(header_values, line.substr(pos));
  message_.header[key] = header_values;
}

void Request::SplitHeaderValues(std::vector<std::string> &splited,
                                const std::string &line) {
  // ','でまずはトークンに分解
  ws_split(splited, line, ',');
  // トークンごとに前後のスペースを削除
  std::vector<std::string>::iterator it = splited.begin();
  while (it != splited.end()) {
    // 前後のスペース全てを削除
    Trim(*it, " \t");
    it++;
  }
}

void Request::Trim(std::string &str, const std::string &delim) {
  // 前後のスペース全てを削除
  std::string::size_type pos = str.find_first_not_of(delim);
  if (pos != std::string::npos) {
    str.erase(0, pos);
  }
  pos = str.find_last_not_of(delim);
  if (pos != std::string::npos) {
    str.erase(pos + 1);
  }
}

void Request::ParseBody(SocketBuff &buffer_) {
  if (!JudgeBodyType()) {
    parse_status_ = ERROR;
    return;
  }
  // chunkedの場合
  if (is_chunked) {
    while (!buffer_.GetString().empty() && parse_status_ != ERROR &&
           parse_status_ != COMPLETE) {
      ParseChunkedBody(buffer_);
    }
  }
  // Content-Lengthの場合
  else {
    ParseContentLengthBody(buffer_);
  }
}

void Request::ParseChunkedBody(SocketBuff &buffer_) {
  if (chunk_status_ == -1) {
    std::string size_str;

    if (!buffer_.GetUntilCRLF(size_str)) {
      parse_status_ = ERROR;
      return;
    }
    errno = 0;
    // size_strを16進数に変換
    chunk_status_ = std::strtol(size_str.c_str(), NULL, 16);
    if (errno == ERANGE) {
      parse_status_ = ERROR;
      return;
    }
    return;
  }

  // chunk_status_が0の場合は、最後のchunk
  if (chunk_status_ == 0) {
    // 次の行がCRLFでない場合は、エラー
    std::string line;
    if (buffer_.GetUntilCRLF(line)) {
      if (line.empty()) {
        parse_status_ = COMPLETE;
        return;
      }
    }
    parse_status_ = ERROR;
    return;
  }

  if (chunk_status_ < 0 || chunk_status_ > static_cast<long>(kMaxBodySize) ||
      chunk_status_ + body_size_ > static_cast<long>(kMaxBodySize)) {
    parse_status_ = ERROR;
    return;
  }

  // bufferの中身が足りない場合 // +2はCRLFの分
  if (static_cast<long>(buffer_.GetBuffSize()) < chunk_status_ + 2) {
    return;
  }
  // 足りてる場合は、追加する文字列を切り取る
  else if (static_cast<long>(buffer_.GetBuffSize()) > chunk_status_ + 2) {
    std::string chunk = buffer_.GetString().substr(0, chunk_status_ + 2);
    // その文字列がCRLFで終わっているかを確認
    // 終わっていない場合は、エラー
    // 終わっている場合は、bodyに追加
    // chunk_status_を-1にする
    std::string::size_type pos = chunk.rfind("\r\n");
    if (pos == std::string::npos) {
      parse_status_ = ERROR;
      return;
    }
    if (pos == chunk.size() - 2) {
      std::string buf = buffer_.GetAndErase(chunk_status_ + 2);
      buf = buf.substr(0, pos);
      message_.body.append(buf);
      chunk_status_ = -1;
    }
  }
}

// 型変えたほうが綺麗そう
void Request::ParseContentLengthBody(SocketBuff &buffer_) {
  if (static_cast<long>(buffer_.GetBuffSize()) == body_size_) {
    message_.body = buffer_.GetAndErase(body_size_);
    parse_status_ = COMPLETE;
  } else {
    // bufferの中身が足りない場合 -> 次のループでまた呼ばれる
    return;
  }
}

bool Request::JudgeBodyType() {
  // headerにContent-LengthかTransfer-Encodingがあるかを調べる
  // どっちもある場合は普通はchunkを優先する
  Header::iterator it_transfer_encoding =
      message_.header.find("Transfer-Encoding");
  Header::iterator it_content_length = message_.header.find("Content-Length");

  // Transfer-Encodingがある場合
  if (it_transfer_encoding != message_.header.end()) {
    std::vector<std::string> values = it_transfer_encoding->second;
    // values を全部探索
    for (std::vector<std::string>::const_iterator it = values.begin();
         it != values.end(); ++it) {
      if (*it == "chunked") {
        is_chunked = true;
        return true;
      }
    }
  }
  // Content-Lengthがある場合
  if (it_content_length != message_.header.end()) {
    std::vector<std::string> values = it_content_length->second;
    // 複数の指定があったらエラー
    // 負の数、strtolのエラーもエラー
    errno = 0;
    long content_length = std::strtol(values[0].c_str(), NULL, 10);
    if (values.size() != 1 || errno == ERANGE || content_length < 0) {
      // error
      return false;
    } else {
      body_size_ = content_length;
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

bool Request::IsLineEnd(const std::string &line, std::string::size_type start) {
  while (start < line.size()) {
    if (std::isspace(line[start]) == false) {
      return false;
    }
    start++;
  }
  return true;
}

bool Request::ValidateRequestSize(std::string &data, size_t max_size) {
  if (data.size() > max_size) {
    return false;
  }
  return true;
}

bool Request::ValidateRequestSize(Header &header, size_t max_size) {
  (void)header;
  if (total_header_size_ > max_size) {
    return false;
  }
  return true;
}

bool Request::ValidateHeaderSize(const std::string &data) {
  if (data.size() > kMaxHeaderLineLength ||
      total_header_size_ + data.size() > kMaxHeaderSize) {
    return false;
  }
  total_header_size_ += data.size();
  return true;
}

bool Request::AssertRequestLine(const std::string &line) {
  if (line.empty() || line.size() > kMaxRequestLineLength) {
    return false;
  }

  // スペースのチェック
  int space_count = std::count(line.begin(), line.end(), ' ');
  if (space_count != 1 && space_count != 2) {
    return false;
  }

  // メソッドのチェック
  std::string::size_type first_space = line.find(' ');
  if (first_space == std::string::npos) {
    return false;
  }
  std::string method = line.substr(0, first_space);
  if (method != "GET" && method != "POST" && method != "DELETE") {
    return false;
  }

  // HTTPのバージョンのチェック (HTTP/0.9の場合は存在しない)
  if (space_count == 2) {
    std::string::size_type last_space = line.rfind(' ');
    if (last_space == std::string::npos || last_space == first_space) {
      return false;
    }
    std::string version = line.substr(last_space + 1);
    if (version != "HTTP/1.0" && version != "HTTP/1.1") {
      return false;
    }
  }

  return true;
}

void Request::SetRequestStatus(Http::HttpStatus status) {
  http_status_ = status;
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
  std::cout << "vserver: " << context_.server_name << std::endl;

  // locationを決定
  ResolveLocation();
  std::cout << "location: " << context_.location.path_ << std::endl;

  Http::DeHexify(context_.resource_path.uri);

  // resource_path, is_cgiを決定
  ResolveResourcePath();
  std::cout << "resource_path: " << context_.resource_path.server_path
            << std::endl;
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
      if (it == path.begin() || it == path.end() - 1) {
        keys.push_front(path.substr(0, it - path.begin() + 1));
      } else {
        keys.push_front(path.substr(0, it - path.begin()));
      }
    }
  }

  // path != "" (uriの分解時に空文字列の場合は'/'としている)
  if (path[path.size() - 1] != '/') {
    keys.push_front(path);
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
  std::string concat = root + path;

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

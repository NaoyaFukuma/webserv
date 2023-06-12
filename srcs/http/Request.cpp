#include "Request.hpp"
#include "utils.hpp"
#include <Socket.hpp>
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <deque>
#include <iostream>

Request::Request() {
  parse_status_ = INIT;
  chunk_status_ = -1;
  total_body_size_ = -1;
  total_header_size_ = 0;
  is_chunked_ = false;
}

Request::~Request() {}

// これがないとcompile時に怒られたので追加
Request::Request(const Request &src) { *this = src; }

Request &Request::operator=(const Request &rhs) {
  if (this != &rhs) {
    message_ = rhs.message_;
    context_ = rhs.context_;
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

void Request::Parse(SocketBuff &buffer_, ConnSocket *socket) {
  std::string line;

  while (parse_status_ != COMPLETE && parse_status_ != ERROR &&
         parse_status_ != BODY && buffer_.GetUntilCRLF(line)) {
    ParseLine(line);
  }
  if (parse_status_ == BODY) {
    ParseBody(buffer_);
  }

  // socket = NULL -> DEBUG用
  if (socket && parse_status_ != ERROR) {
    if (parse_status_ == COMPLETE) {
      ResolvePath(socket->GetConfVec());
    }

    if (!AssertSize()) {
      parse_status_ = ERROR;
      return;
    }

    if (!AssertUrlPath()) {
      parse_status_ = ERROR;
      return;
    }
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
  default:
    break;
  }
}

void Request::ParseRequestLine(const std::string &line) {
  std::vector<std::string> splited;
  if (AssertRequestLine(line) == false) {
    parse_status_ = ERROR;
    return;
  }
  ws_split(splited, line, ' ');

  // HTTP0.9
  message_.request_line.method = splited[0];
  message_.request_line.uri = splited[1];
  if (splited.size() == 2) {
    message_.request_line.version = Http::HTTP09;
    parse_status_ = COMPLETE;
  }
  // HTTP1.0~
  else {
    if (splited[2] == "HTTP/1.0") {
      message_.request_line.version = Http::HTTP10;
    } else if (splited[2] == "HTTP/1.1") {
      message_.request_line.version = Http::HTTP11;
    } else {
      SetRequestStatus(505);
      parse_status_ = ERROR;
      return;
    }
    total_header_size_ = 0;
    parse_status_ = HEADER;
  }
}

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
  if (!SetBodyType()) {
    // content-length,chunkedがない
    if (buffer_.GetString().empty() || chunk_status_ == 0) {
      parse_status_ = COMPLETE;
    } else {
      parse_status_ = ERROR;
    }
  }
  // chunkedの場合
  if (is_chunked_) {
    ParseChunkedBody(buffer_);
  }
  // Content-Lengthの場合
  else {
    ParseContentLengthBody(buffer_);
  }
}

void Request::ParseChunkedBody(SocketBuff &buffer_) {
  while (!buffer_.GetString().empty() && parse_status_ != ERROR &&
         parse_status_ != COMPLETE) {
    if (chunk_status_ == -1) {
      ParseChunkSize(buffer_);
    } else {
      ParseChunkData(buffer_);
    }
  }
}

void Request::ParseChunkSize(SocketBuff &buffer_) {
  std::string size_str;

  if (!buffer_.GetUntilCRLF(size_str)) {
    parse_status_ = ERROR;
    return;
  }
  errno = 0;
  // 文字列を数値に変換
  chunk_status_ = std::strtol(size_str.c_str(), NULL, 16);
  if (errno == ERANGE) {
    parse_status_ = ERROR;
    return;
  }
}

bool Request::AssertSize() {
  size_t client_max_body_size = GetContext().location.client_max_body_size_;
  if (total_body_size_ > client_max_body_size) {
    SetRequestStatus(413);
    return false;
  }
  return true;
}

void Request::ParseChunkData(SocketBuff &buffer_) {
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
      chunk_status_ + total_body_size_ > static_cast<long>(kMaxBodySize)) {
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
    } else {
      SetRequestStatus(400);
      parse_status_ = ERROR;
      return;
    }
  }
}

// 型変えたほうが綺麗そう
void Request::ParseContentLengthBody(SocketBuff &buffer_) {
  if (buffer_.GetBuffSize() >= total_body_size_) {
    message_.body = buffer_.GetAndErase(total_body_size_);
    parse_status_ = COMPLETE;
  } else {
    // bufferの中身が足りない場合 -> 次のループでまた呼ばれる
    return;
  }
}

bool Request::SetBodyType() {
  if (is_chunked_) {
    return true;
  }
  // headerにContent-LengthかTransfer-Encodingがあるかを調べる
  // どっちもある場合は普通はchunkを優先する
  Header::iterator it_transfer_encoding =
      message_.header.find("Transfer-Encoding");
  Header::iterator it_content_length = message_.header.find("Content-Length");

  // Transfer-Encodingがある場合
  if (it_transfer_encoding != message_.header.end()) {
    std::vector<std::string> values = it_transfer_encoding->second;
    if (!values.empty()) {
      // values を全部探索
      for (std::vector<std::string>::const_iterator it = values.begin();
           it != values.end(); ++it) {
        if (*it == "chunked") {
          is_chunked_ = true;
          return true;
        }
      }
    }
  }
  // Content-Lengthがある場合
  if (it_content_length != message_.header.end()) {
    std::vector<std::string> values = it_content_length->second;
    // 複数の指定があったらエラー
    // 負の数、strtolのエラーもエラー
    if (!values.empty()) {
      errno = 0;
      long content_length = std::strtol(values[0].c_str(), NULL, 10);
      if (values.size() != 1 || errno == ERANGE || content_length <= 0) {
        // error
        return false;
      } else {
        total_body_size_ = content_length;
        return true;
      }
    }
  }
  return false;
}

bool Request::ValidateHeaderSize(const std::string &data) {
  if (data.size() > kMaxHeaderLineLength ||
      total_header_size_ + data.size() > kMaxHeaderSize) {
    SetRequestStatus(431);
    return false;
  }
  total_header_size_ += data.size();
  return true;
}

bool Request::AssertUrlPath() {
  // URIの相対path部分がドキュメントルートより上に行っていないかチェック
  // '/'ごとにurlを分割
  std::vector<std::string> split_url;
  if (ws_split(split_url, context_.resource_path.uri.path, '/') < 0) {
    SetRequestStatus(400);
    return false;
  }

  // ".."があるごとに".."とその前の要素を削除
  std::vector<std::string>::iterator it = split_url.begin();
  while (it != split_url.end()) {
    if (*it == "") {
      it = split_url.erase(it);
    } else if (*it == "..") {
      // rootより上に行く場合はエラー
      if (it == split_url.begin()) {
        SetRequestStatus(400);
        return false;
      } else {
        it = split_url.erase(it - 1, it + 1); // ".."とその前の要素を削除
      }
    } else {
      it++;
    }
  }

  return true;
}

bool Request::AssertRequestLine(const std::string &line) {
  if (line.empty() || line.size() > kMaxRequestLineLength) {
    SetRequestStatus(400);
    return false;
  }

  // スペースのチェック
  int space_count = std::count(line.begin(), line.end(), ' ');
  if (space_count != 1 && space_count != 2) {
    SetRequestStatus(400);
    return false;
  }

  // メソッドのチェック
  std::string::size_type first_space = line.find(' ');
  if (first_space == std::string::npos) {
    SetRequestStatus(400);
    return false;
  }
  std::string method = line.substr(0, first_space);
  if (method != "GET" && method != "POST" && method != "DELETE") {
    SetRequestStatus(501);
    return false;
  }

  // URIのチェック
  // HTTP/0.9の場合
  std::string uri;
  if (space_count == 1) {
    uri = line.substr(first_space + 1);
  } else {
    std::string::size_type second_space = line.find(' ', first_space + 1);
    if (second_space == std::string::npos) {
      SetRequestStatus(400);
      return false;
    }
    uri = line.substr(first_space + 1, second_space - first_space - 1);
  }
  if (uri.size() > kMaxUriLength) {
    SetRequestStatus(414);
    return false;
  }

  // HTTPのバージョンのチェック (HTTP/0.9の場合は存在しない)
  if (space_count == 2) {
    std::string::size_type last_space = line.rfind(' ');
    if (last_space == std::string::npos || last_space == first_space) {
      SetRequestStatus(400);
      return false;
    }
    std::string version = line.substr(last_space + 1);
    if (version != "HTTP/1.0" && version != "HTTP/1.1") {
      SetRequestStatus(400);
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
    SetRequestStatus(400);
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
  std::cout << "is_cgi: " << context_.is_cgi << std::endl;
}

std::string Request::ResolveHost() {
  std::string host;
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

    std::cout << "concat: " << concat << std::endl;
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

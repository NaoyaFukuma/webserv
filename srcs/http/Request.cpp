#include "Request.hpp"
#include "utils.hpp"
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <deque>
#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>

Request::Request() {
  parse_status_ = INIT;
  chunk_status_ = -1;
  body_size_ = -1;
  total_header_size_ = 0;
  body_size_ = 0;
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

RequestMessage Request::GetRequestMessage() const { return message_; }

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
      // TODO: ここでエラー処理
      break;
    default:
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

bool Request::ValidateHeaderSize(const std::string &data) {
  if (data.size() > kMaxHeaderLineLength || \
      total_header_size_ + data.size() > kMaxHeaderSize) {
    return false;
  }
  return true;
}

void Request::ParseHeader(const std::string &line) {
  // 空行の場合BODYに移行

  if (IsLineEnd(line, 0)) {
    parse_status_ = BODY;
    return;
  }

  if (!ValidateHeaderSize(line)) {
    // TODO: エラー処理
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
    if (IsLineEnd(line, pos)) {
      break;
    }
    pos = MovePos(line, pos, " \t");
    header_values.push_back(GetWord(line, pos));
  }
  // Headerのkeyとvalueを格納
  message_.header[key] = header_values;
}

void Request::ParseBody(SocketBuff &buffer_) {
  if (!JudgeBodyType()) {
    // TODO: error処理
    return;
  }
// chunkedの場合
  if (chunk_status_ > 0) {
    ParseChunkedBody(buffer_);
  }
    // Content-Lengthの場合
  else {
    ParseContentLengthBody(buffer_);
  }
}

void Request::ParseChunkedBody(SocketBuff &buffer_) {
  while (chunk_status_) {
    std::string str_chunk_size;
    if (!buffer_.GetUntilCRLF(str_chunk_size)) {
      // TODO: BAD_REQUEST
      std::cerr << "BAD_REQUEST" << std::endl;
    }
    if (str_chunk_size == "0") {
      chunk_status_ = -1;
      parse_status_ = COMPLETE;
      break;
    }
    // chunk-sizeを取得
    // ";"があったらその前の数字だけをchunk_sizeにする // オプションのチャンク拡張を追加することができます->無視
    std::string::size_type pos = str_chunk_size.find(';');
    if (pos != std::string::npos) {
      str_chunk_size = str_chunk_size.substr(0, pos);
    }
    // chunk-sizeを16進数から10進数に変換
    errno = 0;
    unsigned long long chunk_size = std::strtoull(str_chunk_size.c_str(), NULL, 16);
    // TODO: あとボディのサイズ超えてたりしたらエラーにする
    if (errno == ERANGE) {
      // TODO: BAD_REQUEST
      std::cerr << "BAD_REQUEST" << std::endl;
      return;
    }
    // chunk-size分だけbodyに追加
    std::string data;
    if (!buffer_.GetUntilCRLF(data) || data.size() != chunk_size) {
      // TODO: BAD_REQUEST
      std::cerr << "BAD_REQUEST" << std::endl;
    }
    message_.body.append(data);
  }
}

// 型変えたほうが綺麗そう
void Request::ParseContentLengthBody(SocketBuff &buffer_) {
  // buffer_の内容をとってくる
  std::string buffer = buffer_.GetString();
  if (body_size_ > 0) {
    // content-lengthまでに長さを制限
    size_t read_size = static_cast<long long >(buffer.size()) > body_size_ ? body_size_ : buffer.size();
    // 長さ分だけ追加
    message_.body.append(buffer, 0, read_size);
    body_size_ -= static_cast<long long>(read_size);
  }
  if (body_size_ < 0) {
    parse_status_ = COMPLETE;
  }
}

bool Request::JudgeBodyType() {
  // headerにContent-LengthかTransfer-Encodingがあるかを調べる
  // どっちもある場合は普通はchunkを優先する
  Header::iterator it_transfer_encoding = message_.header.find("Transfer-Encoding");
  Header::iterator it_content_length = message_.header.find("Content-Length");

  // Transfer-Encodingがある場合
  if (it_transfer_encoding != message_.header.end()) {
    std::vector<std::string> values = it_transfer_encoding->second;
    // values を全部探索
    for (std::vector<std::string>::const_iterator it = values.begin();
         it != values.end(); ++it) {
      if (*it == "chunked") {
        // この変数ってchunkかのフラグではない？
        chunk_status_ = true;
        return true;
      }
    }
  }
  // Content-Lengthがある場合
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

bool Request::SplitRequestLine(std::vector<std::string> &splited, const std::string &line) {
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
        if (ExistCgiFile(partial_path, cgi_extension)) {
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
        ExistCgiFile(concat, cgi_extension)) {
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

// partial_pathがcgi_extensionで終わり、かつregular
// fileであれば、cgiとして実行する
bool Request::ExistCgiFile(const std::string &path,
                           const std::string &extension) const {
  struct stat path_stat;
  stat(path.c_str(), &path_stat);
  return (extension == "." || end_with(path, extension)) &&
         S_ISREG(path_stat.st_mode);
}

// for unit-test
void Request::SetMessage(RequestMessage message) { message_ = message; }

#include "Request.hpp"
#include "utils.hpp"
#include <cerrno>
#include <stdlib.h>

Request::Request() {
  parse_status_ = INIT;
  chunk_status_ = -1;
  body_size_ = -1;
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

#include <iostream>

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
    if (IsLineEnd(line, pos)) {
      break;
    }
    pos = MovePos(line, pos, " \t");
    header_values.push_back(GetWord(line, pos));
  }
  // Headerのkeyとvalueを格納
  message_.header[key] = header_values;
}

void Request::ParseBody(const std::string &line) {
  if (!JudgeBodyType()) {
    // TODO: error処理
    return;
  }
// chunkedの場合
    if (chunk_status_ > 0) {
        ParseChunkedBody(line);
    }
        // Content-Lengthの場合
    else {
        ParseContentLengthBody(line);
    }
}

void Request::ParseChunkedBody(const std::string &line) {
  (void )line;
}

void Request::ParseContentLengthBody(const std::string &line) {
  if (line.size() <= kMaxBodySize) {
    message_.body += line;
  } else {
    message_.body += line.substr(0, kMaxBodySize);
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
    for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it) {
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


std::string Request::GetWord(const std::string &line, std::string::size_type &pos) {
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

std::string::size_type
Request::MovePos(const std::string &line, std::string::size_type start, const std::string &delim) {
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
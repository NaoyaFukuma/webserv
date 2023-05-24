#include "CgiSocket.hpp"
#include "CgiResponseParser.hpp"
#include "Epoll.hpp"
#include "Http.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Socket.hpp"
#include "define.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <wait.h>

CgiResponseParser::CgiResponseParser(CgiSocket &cgi_socket)
    : cgi_socket_(cgi_socket) {}

// recv_buffer_内のCGIレスポンスからResponseを構築する
Response CgiResponseParser::ParseCgiResponse() {
  size_t header_len = 0;     // ヘッダーの総合計長 maxは32KB
  bool is_body = false;      // ボディを持つかどうか
  size_t content_length = 0; // ボディの長さ maxは1MB
  Response http_response;    // 成果物の入れ物

  // 一度のParseで完結するので先にDONEを設定
  // TODO:

  // HTTPリクエストからHTTP versionを取得し、response_messageに設定
  // response_message.version_ =
  //     this->http_request_.GetRequestMessage().request_line.version;

  // recv_buffer_から直接1行づつ取得する
  std::string line;
  while (this->cgi_socket_.recv_buffer_.GetUtillCRLF(line)) {
    // 1行が空行だったら、ヘッダーの終わりなので、ヘッダーの終わりを設定する
    if (line.empty()) {
      break;
    }

    if (!IsValidtHeaderLine(header_len, line.length())) {
      // 500 Internal Server Error
      SetInternalServerError(response_message);
      break;
    }

    //key と valueに分ける
    std::pair<std::string, std::string> header_pair = splitHeader(line);
    // valueを','で分ける
    std::vector<std::string> header_values = splitHeaderValue(header_pair.second);

    // keyによって処理を分ける
    if (header_pair.first == "Status") { // statusの場合

    } else if () { // locationの場合
      // local redirect

      // client redirect

    } else if () { // content-typeの場合

    } else if () { // content-lengthの場合
      is_body = true;

    } else if () { // set-cookieの場合

    } else { // それ以外の場合は、そのままresponse_に設定

    }
  }

  // ボディがあれば、ボディを取得してresponse_messageに設定する。長さも確認
  if (is_body) {

  }
  return http_response;
}

// parserメソッド郡



// IsValidメソッド郡
bool CgiResponseParser::IsValidHeaderLine(const std::string &line, size_t &header_len) {
  if (line.empty()) {
    std::cerr << "Keep Running Error: CGI response header line is empty"
              << std::endl;
    return false;
  }
  if (!IsValidtHeaderLineLength(line)) {
    return false;
  }
  if (!IsValidtHeaderLength(header_len, line.length())) {
    return false;
  }
  if (!IsValidtHeaderLineFormat(line)) {
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidtHeaderLineLength(const std::string &line) {
  if (line.size() > this->kMaxHeaderLineLength) {
    std::cerr << "Keep Running Error: CGI response header line length too long"
              << std::endl;
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidtHeaderLength(size_t &header_len,
                                             size_t line_len) {
  header_len += line_len;
  if (header_len > this->kMaxHeaderLength) {
    std::cerr << "Keep Running Error: CGI response header length too long"
              << std::endl;
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidtHeaderLineFormat(const std::string &line) {
  // ':'の数を数えて、一つだけあることを確認
  int colon_count = std::count(line.begin(), line.end(), ':');
  if (colon_count != 1) {
    std::cerr << "Keep Running Error: CGI response header line format error"
              << std::endl;
    return false;
  }
  std::pair<std::string, std::string> kv = splitHeader(line);
  return isValidHeaderKey(kv.first) && isValidHeaderValue(kv.second);
}

bool CgiResponseParser::IsValidHeaderKey(const std::string &key) {
  // Key should be a valid token
  return IsValidToken(key);
}

bool CgiResponseParser::IsValidHeaderValue(const std::string &value) {
  // Value can contain arbitrary TEXT, so we don't need to validate it here
  return true;
}

bool CgiResponseParser::IsValidToken(const std::string &token) {
  // Token must not be empty
  if (token.empty())
    return false;

  // HTTPヘッダーフィールドのkeyで使える有効な文字種
  const std::string validChars =
      "!#$%&'*+-.^_`|~"
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  // 有効な文字種以外のポジションを返す、つまりnpos以外が返ってきたら、
  // 有効な文字種以外があるということなので、falseを返す
  return token.find_first_not_of(validChars) == std::string::npos;
}

// Parseメソッド郡
bool CgiResponseParser::ParseHeader(const std::string &line, Header &header) {
  std::pair<std::string, std::string> kv = splitHeader(line);
  header.key = kv.first;
  header.value = kv.second;
  return true;

}

// HTTPレスポンスを構築するメソッド郡
void CgiResponseParser::SetInternalErrorResponse(
    ResponseMessage &response_message) {
  // TODO: tfujiwara
}



// utilityメソッド郡
std::pair<std::string, std::string>
CgiResponseParser::splitHeader(const std::string &headerLine) {
  std::string::size_type pos = headerLine.find(':');
  if (pos != std::string::npos) {
    std::string key = headerLine.substr(0, pos);
    std::string value = headerLine.substr(pos + 1);
    return std::make_pair(trim(key), trim(value));
  } else {
    throw std::invalid_argument("Invalid header line: no ':' character");
  }
}

std::vector<std::string> splitValue(const std::string &value) {
  std::istringstream iss(value);
  std::string item;
  std::vector<std::string> result;
  while (std::getline(iss, item, ',')) {
    result.push_back(trim(item));
  }
  return result;
}

// 前後の空白文字(半角スペースと水平タブ)を削除するtrim関数
std::string trim(const std::string &str) {
  const std::size_t strBegin = str.find_first_not_of(" \t");
  if (strBegin == std::string::npos)
    return ""; // 空白文字のみの場合は空文字列を返す
  const std::size_t strEnd = str.find_last_not_of(" \t");
  const std::size_t strRange = strEnd - strBegin + 1;
  return str.substr(strBegin, strRange);
}

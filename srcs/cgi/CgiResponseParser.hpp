#ifndef CGI_RESPONSE_PARSER_HPP_
#define CGI_RESPONSE_PARSER_HPP_

#include "Request.hpp"
#include "CgiSocket.hpp"
#include "Response.hpp"
#include <string>

class CgiResponseParser {
private:
  CgiSocket &cgi_socket_;  // CGIと通信したソケット
  Response http_response_; // http response

  // 各種長さの制限に使う定数
  // ヘッダー１行の最大文字数 8KB = 8 * 1024 = 8192
  static const int kMaxHeaderLineLength = 8192;
  // ヘッダー全体の最大文字数 32KB = 32 * 1024 = 32768
  static const int kMaxHeaderLength = 32768;
  // ボディの最大文字数 1MB = 1024 * 1024 = 1048576
  static const int kMaxBodyLength = 1048576;

public:
  // ---------  in CgiResponseParser.cpp  ------------------
  CgiResponseParser(CgiSocket &cgi_socket);
  ~CgiResponseParser();
  void ParseCgiResponse();

  // parseの結果を返すメソッド郡
  bool IsRedirectCgi();
  Response GetHttpResponse();
  Request GetHttpRequestForCgi();

private:
  // parserメソッド郡
  void ParseLocationPath(std::string &path);

  // IsValidメソッド郡
  bool IsValidHeaderLine(const std::string &line, size_t &header_len);
  bool IsValidtHeaderLineLength(const std::string &line);
  bool IsValidtHeaderLength(size_t &header_len, size_t line_len);
  bool IsValidtHeaderLineFormat(const std::string &line);
  bool IsValidHeaderKey(const std::string &key);
  bool IsValidHeaderValue(const std::string &value);
  bool IsValidToken(const std::string &token);

  // utilityメソッド郡
  std::pair<std::string, std::string>
    splitHeader(const std::string &headerLine);
  std::vector<std::string> splitValue(const std::string &value);
  std::string trim(const std::string &str);


private:
  CgiResponseParser(); // デフォルトコンストラクタ禁止
  CgiResponseParser(const CgiResponseParser &src); // コピーコンストラクタ禁止
  CgiResponseParser &operator=(const CgiResponseParser &src); // 代入演算子禁止
};

#endif // CGI_RESPONSE_PARSER_HPP_

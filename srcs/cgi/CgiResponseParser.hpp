#ifndef CGI_RESPONSE_PARSER_HPP_
#define CGI_RESPONSE_PARSER_HPP_

#include "CgiSocket.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include <string>

class CgiResponseParser {
public:
  enum ParseResult {
    CREATED_HTTP_RESPONSE,
    RIDIRECT_TO_LOCAL_CGI,
  };
  enum LedirectType {
    NO_REDIRECT,
    LOCAL_STATIC_FILE_REDIRECT,
    LOCAL_CGI_REDIRECT,
    CLIENT_REDIRECT,
  };

private:
  CgiSocket &cgi_socket_;
  const Request src_http_request_;
  Response &dest_http_response_;
  Response temp_http_response_;
  Context temp_context_;
  ParseResult parse_result_;
  LedirectType redirect_type_;
  CgiSocket *redirect_new_cgi_socket_;
  Epoll *epoll_;

  // 各種長さ制限
  static const int kMaxHeaderLineLength =
      8192; // ヘッダー１行 8KB = 8 * 1024 = 8192
  static const int kMaxHeaderLength =
      32768; // ヘッダー全体 32KB = 32 * 1024 = 32768
  static const int kMaxBodyLength = 1048576; // 1MB = 1024 * 1024 = 1048576

public:
  CgiResponseParser(CgiSocket &cgi_socket, const Request http_request,
                    Response &http_response, Epoll *epoll);
  ~CgiResponseParser();
  void ParseCgiResponse();

  ParseResult GetParseResult() const { return parse_result_; }
  CgiSocket *GetRedirectNewCgiSocket() const {
    return redirect_new_cgi_socket_;
  }

private:
  void SetInternalErrorHttpResponse();

  // parserメソッド
  int ParseHeader();
  int ParseBody();
  int ParseStatusHeader();
  int ParseLocationHeader();
  int ParseLocationHeaderValue(std::vector<std::string> &location_header_value);
  int ParseLocalRedirect(std::string &redirect_path);
  // parse内で呼び出すメソッド
  int GetAndSetLocalStaticFile();
  int CreateNewCgiSocketProcess();
  Request CreateNewCgiRequest();

  // IsValidメソッド
  bool IsValidHeaderLine(const std::string &line, std::size_t &header_len);
  bool IsValidtHeaderLineLength(const std::string &line);
  bool IsValidHeaderLength(std::size_t &header_len, std::size_t line_len);
  bool IsValidHeaderLineFormat(const std::string &line);
  bool IsValidHeaderKey(const std::string &key);
  bool IsValidHeaderValue(const std::string &value);
  bool IsValidToken(const std::string &token);
  bool IsValidBodyLength(std::size_t body_len);
  bool IsValidStatusCode(const int status_code);
  bool IsValidAbsolutePath(const std::string &path);

  // utilityメソッド
  std::pair<std::string, std::string>
  SplitHeader(const std::string &headerLine);
  std::vector<std::string> SplitValue(const std::string &value);
  std::string trim(const std::string &str);

private:
  CgiResponseParser(); // デフォルトコンストラクタ禁止
  CgiResponseParser(const CgiResponseParser &src); // コピーコンストラクタ禁止
  CgiResponseParser &operator=(const CgiResponseParser &src); // 代入演算子禁止
};

#endif // CGI_RESPONSE_PARSER_HPP_

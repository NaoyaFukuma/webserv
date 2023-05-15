#ifndef _REQUEST_HPP_
#define _REQUEST_HPP_

#include "Header.hpp"
#include "IOBuff.hpp"
#include <map>
#include <string>
#include <vector>

struct RequestMessage {
  RequestLine request_line;
  Header header;
  std::string body;
};

struct RequestLine {
  std::string method;
  std::string uri;
  HTTPVersion version;
};

enum HTTPVersion {
  HTTP09,
  HTTP10,
  HTTP11,
};

struct Header {
  // General Header
  std::string Accept;
  std::string AcceptCharset;
  std::string AcceptEncoding;
  std::string AcceptLanguage;
  std::string Authorization;
  std::string From;
  std::string Host;
  std::string IfModifiedSince;
  std::string IfUnmodifiedSince;
  std::string IfMatch;
  std::string IfNoneMatch;
  std::string IfRange;
  std::string MaxForwards;
  std::string ProxyAuthorization;
  std::string Range;
  std::string Referer;
  std::string UserAgent;

  // Request Header
  std::string Connection;
  std::string Date;
  std::string Pragma;
  std::string TransferEncoding;
  std::string Upgrade;
  std::string Via;

  // Entity Header
  std::string Allow;
  std::string ContentBase;
  std::string ContentEncoding;
  std::string ContentLanguage;
  int ContentLength;
  std::string ContentLocation;
  std::string ContentMD5;
  std::string ContentRange;
  std::string ContentType;
  std::string Etag;
  std::string LastModified;
};

enum ParseStatus {
  INIT,
  HEADER,
  BODY,
  COMPLETE,
  ERROR,
};

struct HttpError {
  int status_code;
  std::string message;

private:
  const static std::map<int, std::string> default_error_message =
      init_default_error_message();

  HttpError(int status_code, std::string message)
      : status_code(status_code), message(message) {}
}

class Request {
private:
  RequestMessage message_;
  ParseStatus parse_status_;
  HttpError error_status_;
  int chunk_status_; // chunkでbodyを受け取るとき、前の行を覚えておくための変数

  void ParseLine(const std::string &line);
  void ParseRequestLine(const std::string &line);
  void ParseHeader(const std::string &line);
  void ParseBody(const std::string &line);

  void SetError(int status, std::string message);

public:
  Request();
  ~Request();
  Request &operator=(const Request &rhs);

  RequestMessage GetRequestMessage() const;
  ParseStatus GetParseStatus() const;
  int GetErrorStatus() const;
  void Parse(IOBuff &buffer_);
  void Clear();

private: // 使用予定なし
  Request(const Request &src);
};

std::ostream &operator<<(std::ostream &os, const Request &request);

std::map<int, string> init_default_error_message();

#endif
#ifndef _REQUEST_HPP_
#define _REQUEST_HPP_

#include "Config.hpp"
#include "SocketBuff.hpp"
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

typedef std::map<std::string, std::vector<std::string>> Header;

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
  const static std::map<int, std::string> kDefaultErrorMessage;

  HttpError(int status_code, std::string message)
      : status_code(status_code), message(message) {}
};

struct ResourcePath {
  std::string server_path;
  std::string query;
  std::string path_info;
};

class Request {
private:
  RequestMessage message_;
  ParseStatus parse_status_;
  HttpError error_status_;
  int chunk_status_; // chunkでbodyを受け取るとき、前の行を覚えておくための変数
  static const size_t kMaxHeaderSize = 8192; // 8KB

  Vserver *vserver_;
  Location *location_;
  ResourcePath resource_path_;

  void ParseLine(const std::string &line);
  void ParseRequestLine(const std::string &line);
  void ParseHeader(const std::string &line);
  void ParseBody(const std::string &line);

  void SetError(int status, std::string message);
  void SetError(int status);

public:
  Request();
  ~Request();
  Request &operator=(const Request &rhs);

  RequestMessage GetRequestMessage() const;
  ParseStatus GetParseStatus() const;
  int GetErrorStatus() const;
  void Parse(SocketBuff &buffer_);
  void Clear();

private: // 使用予定なし
  Request(const Request &src);
};

std::ostream &operator<<(std::ostream &os, const Request &request);

std::map<int, std::string> init_default_error_message();

#endif
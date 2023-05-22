#ifndef REQUEST_HPP_
#define REQUEST_HPP_

#include "Config.hpp"
#include "Http.hpp"
#include "SocketBuff.hpp"
#include <map>
#include <string>
#include <vector>

struct RequestLine {
  std::string method;
  std::string uri;
  Http::Version version;
};

typedef std::map<std::string, std::vector<std::string> > Header;

struct RequestMessage {
  RequestLine request_line;
  Header header;
  std::string body;
};

enum ParseStatus {
  INIT,
  HEADER,
  BODY,
  COMPLETE,
  ERROR,
};

struct ResourcePath {
  Http::URI uri;
  std::string server_path;
  std::string path_info;
};

struct Context {
  Vserver *vserver;
  Location *location;
  ResourcePath resource_path;
  bool is_cgi;
};

class Request {
private:
  RequestMessage message_;
  ParseStatus parse_status_;
  Http::HttpError error_status_;
  int chunk_status_; // chunkでbodyを受け取るとき、前の行を覚えておくための変数
  static const size_t kMaxHeaderSize = 8192; // 8KB

  Context context_; // ResolvePath()で設定される
// TESTがdefineされている場合はpublicにする
#ifdef DEBUG
public:
#endif
  void ParseLine(const std::string &line);
  void ParseRequestLine(const std::string &line);
  void ParseHeader(const std::string &line);
  void ParseBody(const std::string &line);

  void SetError(int status, std::string message);
  void SetError(int status);

  std::string::size_type MovePos(const std::string &line, std::string::size_type start, const std::string& delim);

public:
  Request();
  ~Request();
  Request(const Request &src);
  Request &operator=(const Request &rhs);

  RequestMessage GetRequestMessage() const;
  ParseStatus GetParseStatus() const;
  Http::HttpError GetErrorStatus() const;
  void Parse(SocketBuff &buffer_);
  void Clear();
  void ResolvePath(const Config &config);

  Context GetContext() const;
  Header GetHeaderMap() const;
};

std::ostream &operator<<(std::ostream &os, const Request &request);

std::map<int, std::string> init_default_error_message();

#endif

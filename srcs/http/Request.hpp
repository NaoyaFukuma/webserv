#ifndef REQUEST_HPP_
#define REQUEST_HPP_

#include "Config.hpp"
#include "Http.hpp"
#include "SocketBuff.hpp"
#include <map>
#include <string>
#include <vector>

class ConnSocket;

struct RequestLine {
  RequestLine() : version(Http::HTTP11) {}
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
  Context() : is_cgi(false) {}
  Vserver vserver;
  std::string server_name;
  Location location;
  ResourcePath resource_path;
  bool is_cgi;
};

class Request {
private:
  RequestMessage message_;
  ParseStatus parse_status_;
  Http::HttpStatus http_status_;
  bool is_chunked_;
  long chunk_status_; // chunkでbodyを受け取るとき、前の行を覚えておくための変数
  std::size_t total_header_size_; // ヘッダーの長さを覚えておくための変数
  std::size_t total_body_size_; // bodyの長さを覚えておくための変数
  static const std::size_t kMaxHeaderLineLength =
      8192; // 8KB // ヘッダーの1行の最大長さ
  static const std::size_t kMaxHeaderSize =
      32768; // 32KB // ヘッダー全体の最大長さ
  static const std::size_t kMaxBodySize = 1048576; // 1MB // bodyの最大長さ
  static const std::size_t kMaxUriLength = 2048;
  static const std::size_t kMaxRequestLineLength =
      8192; // 8KB // リクエストラインの最大長さ

  Context context_; // ResolvePath()で設定される

// DEBUGがdefineされている場合はpublicにする
#ifdef DEBUG
public:
#endif

  // parse系
  void ParseLine(const std::string &line);
  void ParseRequestLine(const std::string &line);
  void ParseHeader(const std::string &line);
  void ParseBody(SocketBuff &buffer_);
  void ParseChunkedBody(SocketBuff &buffer_);
  void ParseChunkSize(SocketBuff &buffer_);
  void ParseChunkData(SocketBuff &buffer_);
  void ParseContentLengthBody(SocketBuff &buffer_);

  // parse utils
  void SplitHeaderValues(std::vector<std::string> &splited,
                         const std::string &line);
  void Trim(std::string &str, const std::string &delim);
  bool SetBodyType();
  bool FindTransferEncoding();
  bool FindContentLength();
  bool AssertRequestLine(const std::string &line);
  bool AssertUrlPath();
  bool AssertSize() const;
  bool AssertAllowMethod();
  bool CompareMethod(const std::string &lhs, enum method_type rhs) const;

  // resolve系
  std::string ResolveHost();
  void ResolveVserver(const ConfVec &vservers, const std::string &host);
  void ResolveLocation();
  void ResolveResourcePath();
  bool ValidateHeaderSize(const std::string &data);

public:
  Request();
  ~Request();
  Request(const Request &src);
  Request &operator=(const Request &rhs);

  RequestMessage GetRequestMessage() const;
  ParseStatus GetParseStatus() const;
  void Parse(SocketBuff &buffer_, ConnSocket *socket = NULL);
  void ResolvePath(const ConfVec &vservers);
  Context GetContext() const;
  Header GetHeaderMap() const;
  void SetRequestStatus(Http::HttpStatus status);
  std::vector<std::string> GetHeader(const std::string &key);
  Http::HttpStatus GetRequestStatus() const;
  bool HasHeader(const std::string &key) const;

  // for test
  void SetParseStatus(ParseStatus status) { parse_status_ = status; }
  long GetChunkStatus() const { return is_chunked_; }
  std::size_t GetContentLength() const { return total_body_size_; }
  std::string GetBody() const { return message_.body; }

  // for unit-test
  void SetMessage(RequestMessage message);
  void SetContext(Context context);
};

std::ostream &operator<<(std::ostream &os, const Request &request);

#endif

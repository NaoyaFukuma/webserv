#ifndef REQUEST_HPP_
#define REQUEST_HPP_

#include "Config.hpp"
#include "Http.hpp"
#include "SocketBuff.hpp"
#include <map>
#include <string>
#include <vector>

struct RequestLine {
  std::string method;    // そのまま
  std::string uri;       // 更新
  Http::Version version; // そのまま
};

typedef std::map<std::string, std::vector<std::string> > Header;

struct RequestMessage {
  RequestLine request_line; // 一部更新
  Header header;            // 更新
  std::string body;         // そのまま
};

enum ParseStatus {
  INIT,
  HEADER,
  BODY,
  COMPLETE,
  ERROR,
};

struct ResourcePath {
  Http::URI uri;           // 更新
  std::string server_path; // 更新
  std::string path_info;   // 更新
};

struct Context {
  Vserver vserver;            // そのまま
  std::string server_name;    // そのまま
  Location location;          // そのまま
  ResourcePath resource_path; // 更新
  bool is_cgi;                // 更新
};

class Request {
private:
    RequestMessage message_;
    ParseStatus parse_status_;
    Http::HttpStatus http_status_;
    bool is_chunked_;
    long chunk_status_; // chunkでbodyを受け取るとき、前の行を覚えておくための変数
    size_t total_header_size_; // ヘッダーの長さを覚えておくための変数
    long body_size_; // bodyの長さを覚えておくための変数
    static const size_t kMaxHeaderLineLength = 8192; // 8KB // ヘッダーの1行の最大長さ
    static const size_t kMaxHeaderSize = 32768; // 32KB // ヘッダー全体の最大長さ
    static const size_t kMaxBodySize = 1048576; // 1MB // bodyの最大長さ
    static const size_t kMaxUriLength = 2048;
    static const size_t kMaxRequestLineLength = 8192; // 8KB // リクエストラインの最大長さ

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

  void SplitHeaderValues(std::vector<std::string> &splited, const std::string &line);
  void Trim(std::string &str, const std::string &delim);
  bool SetBodyType();

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
  void Parse(SocketBuff &buffer_);
  void ResolvePath(const ConfVec &vservers);
  Context GetContext() const;
  Header GetHeaderMap() const;

  // for test
  void SetParseStatus(ParseStatus status) { parse_status_ = status; }
  int GetChunkStatus() const { return is_chunked_; }
  long long GetContentLength() const { return body_size_; }
  std::string GetBody() const { return message_.body; }
  void SetError(int status, std::string message);
  void SetError(int status);

  // for unit-test
  void SetMessage(RequestMessage message);
  void SetContext(Context context);

  /* 移動させます */
  bool AssertRequestLine(const std::string &line);
  void SetRequestStatus(Http::HttpStatus status);
  bool HasHeader(const std::string &key) const;
  std::vector<std::string> GetHeader(const std::string &key);
  Http::HttpStatus GetRequestStatus() const;

};

std::ostream &operator<<(std::ostream &os, const Request &request);

#endif

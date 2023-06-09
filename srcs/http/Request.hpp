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
  bool is_chunked;
  long chunk_status_; // chunkでbodyを受け取るとき、前の行を覚えておくための変数
  std::size_t total_header_size_; // ヘッダーの長さを覚えておくための変数
  long body_size_;           // bodyの長さを覚えておくための変数
  static const std::size_t kMaxHeaderLineLength =
      8192; // 8KB // ヘッダーの1行の最大長さ
  static const std::size_t kMaxHeaderSize = 32768; // 32KB // ヘッダー全体の最大長さ
  static const std::size_t kMaxBodySize = 1048576; // 1MB // bodyの最大長さ
  static const std::size_t kMaxUriLength = 2048;
  static const std::size_t kMaxRequestLineLength =
      8192; // 8KB // リクエストラインの最大長さ

  Context context_; // ResolvePath()で設定される

// DEBUGがdefineされている場合はpublicにする
#ifdef DEBUG
public:
#endif

  void ParseLine(const std::string &line);
  void ParseRequestLine(const std::string &line);
  void ParseHeader(const std::string &line);
  void ParseBody(SocketBuff &buffer_);
  std::string::size_type MovePos(const std::string &line,
                                 std::string::size_type start,
                                 const std::string &delim);
  bool IsLineEnd(const std::string &line, std::string::size_type pos);
  void SplitHeaderValues(std::vector<std::string> &splited,
                         const std::string &line);
  void Trim(std::string &str, const std::string &delim);
  bool SplitRequestLine(std::vector<std::string> &splited,
                        const std::string &line);
  // 名前が微妙
  bool JudgeBodyType();
  void ParseChunkedBody(SocketBuff &buffer_);
  void ParseContentLengthBody(SocketBuff &buffer_);

  std::string ResolveHost();
  void ResolveVserver(const ConfVec &vservers, const std::string &host);
  void ResolveLocation();
  void ResolveResourcePath();
  bool ExistCgiFile(const std::string &path,
                    const std::string &extension) const;

public:
  Request();
  ~Request();
  Request(const Request &src);
  Request &operator=(const Request &rhs);

  RequestMessage GetRequestMessage() const;
  ParseStatus GetParseStatus() const;
  //    Http::HttpError GetErrorStatus() const;
  void Parse(SocketBuff &buffer_);
  void Clear();
  void ResolvePath(const ConfVec &vservers);
  Context GetContext() const;
  Header GetHeaderMap() const;
  std::string GetWord(const std::string &line, std::string::size_type &pos);
  bool ValidateRequestSize(std::string &data, std::size_t max_size);
  bool ValidateRequestSize(Header &header, std::size_t max_size);
  bool ValidateHeaderSize(const std::string &data);

  // for test
  void SetParseStatus(ParseStatus status) { parse_status_ = status; }
  int GetChunkStatus() const { return is_chunked; }
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

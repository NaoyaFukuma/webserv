#ifndef RESPONSE_HPP_
#define RESPONSE_HPP_

#include "Config.hpp"
#include "Http.hpp"
#include "Request.hpp"
#include <ctime>
#include <dirent.h>
#include <map>
#include <string>

class ConnSocket;
class Epoll;

enum ProcessStatus {
  PROCESSING,
  DONE,
};

typedef std::vector<std::pair<std::size_t, std::size_t>> RangeVec;

class Response {
private:
  ProcessStatus process_status_;

  Http::Version version_;
  int status_code_;
  std::string status_message_;
  Header header_;
  std::string body_;

  RangeVec ranges_;

  // 各種長さの制限に使う定数
  // ヘッダー１行の最大文字数 8KB = 8 * 1024 = 8192
  static const int kMaxHeaderLineLength = 8192;
  // ヘッダー全体の最大文字数 32KB = 32 * 1024 = 32768
  static const int kMaxHeaderLength = 32768;
  // ボディの最大文字数 1MB = 1024 * 1024 = 1048576
  static const int kMaxBodyLength = 1048576;

  void ProcessCgi(Request &request, ConnSocket *socket, Epoll *epoll);
  void ProcessStatic(Request &request, ConnSocket *socket, Epoll *epoll);

  void ProcessReturn(Request &request, ConnSocket *socket, Epoll *epoll);
  void ProcessGET(Request &request);
  void ProcessDELETE(Request &request);

  void GetFile(Request &request, const std::string &path);
  bool StaticFileBody(const std::string &path);
  bool IsGetableFile(Request &request, const std::string &path);

  void DeleteFile(Request &request, const std::string &path);
  bool IsDeleteableFile(Request &request, const std::string &path);

  void ProcessAutoindex(Request &request, const std::string &path);
  void ResFileList(DIR *dir);

  bool IfModSince(Request &request, const std::string &path);
  bool IfUnmodSince(Request &request, const std::string &path);
  bool IfMatch(Request &request, const std::string &path);
  bool IfNone(Request &request, const std::string &path);
  bool IfRange(Request &request, const std::string &path);
  bool FindRanges(Request &request, const std::string &path);

  std::time_t GetLastModified(const std::string &path);
  std::string GetEtag(const std::string &path);

public:
  Response();
  ~Response();
  Response(const Response &src);
  Response &operator=(const Response &src);

  std::string GetString();
  ProcessStatus GetProcessStatus() const;
  std::vector<std::string> GetHeader(const std::string &key);
  bool HasHeader(const std::string &key) const;

  void SetResponseStatus(Http::HttpStatus status);
  void SetVersion(Http::Version version);
  void SetHeader(const std::string &key,
                 const std::vector<std::string> &values);
  void SetBody(std::string body);

  void ProcessRequest(Request &request, ConnSocket *socket, Epoll *epoll);
};

#endif // RESPONSE_HPP_

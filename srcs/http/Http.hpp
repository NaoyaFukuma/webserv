#ifndef HTTP_HPP_
#define HTTP_HPP_
#include <map>
#include <string>

namespace Http {
struct URI {
  std::string scheme;
  std::string user;
  std::string pass;
  std::string host;
  std::string port;
  std::string path;
  std::string query;
  std::string fragment;
};

enum Version {
  HTTP09,
  HTTP10,
  HTTP11,
};

std::string VersionToString(Version version);

struct HttpStatus {
  int status_code;
  std::string message;

private:
  const static std::map<int, std::string> kDefaultStatusMessage;

public:
  HttpStatus();
  HttpStatus(int status_code, std::string message);
  // Todo: messageはkDefaultErrorMessageから取得するようにする
  // Todo: HTTPErrorの時のheaderやbodyはどうなるかを調査する
  HttpStatus(int status_code);
};

std::map<int, std::string> init_default_error_message();

bool SplitURI(URI &dst, const std::string &src);
std::string DeHexify(std::string uri);
void DeHexify(Http::URI &uri);
bool IsHexDigit(char c);
int Hex2Char(char c);
}; // namespace Http

#endif // HTTPUTILS_HPP_
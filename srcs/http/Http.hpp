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

struct HttpError {
  int status_code;
  std::string message;

private:
  const static std::map<int, std::string> kDefaultErrorMessage;

public:
  HttpError() : status_code(-1), message("") {}

  HttpError(int status_code, std::string message)
      : status_code(status_code), message(message) {}
};

bool SplitURI(URI &dst, const std::string &src);
std::string DeHexify(std::string uri);
void DeHexify(Http::URI &uri);
bool IsHexDigit(char c);
int Hex2Char(char c);
}; // namespace Http

#endif // HTTPUTILS_HPP_
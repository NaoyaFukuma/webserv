#ifndef HTTP_HPP_
#define HTTP_HPP_
#include <string>

namespace Http {
struct URI {
  std::string scheme;
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

bool SplitURI(URI &dst, const std::string &src);
std::string DeHexify(std::string uri);
bool IsHexDigit(char c);
int Hex2Char(char c);
}; // namespace Http

#endif // HTTPUTILS_HPP_
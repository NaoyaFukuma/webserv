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
}; // namespace Http

#endif // HTTPUTILS_HPP_
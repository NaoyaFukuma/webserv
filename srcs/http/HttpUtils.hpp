#ifndef HTTPUTILS_HPP_
#define HTTPUTILS_HPP_
#include <string>

namespace HttpUtils {
struct URI {
  std::string scheme;
  std::string host;
  std::string port;
  std::string path;
  std::string query;
  std::string fragment;
};

bool SplitURI(URI &dst, const std::string &src);
}; // namespace HttpUtils

#endif // HTTPUTILS_HPP_
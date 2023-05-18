#include "HttpUtils.hpp"

bool HttpUtils::SplitURI(URI &dst, const std::string &src) {
  std::size_t scheme_pos = src.find("://");
  if (scheme_pos != std::string::npos) {
    // absolute URI
    dst.scheme = src.substr(0, scheme_pos);
    std::size_t host_start = scheme_pos + 3;
    std::size_t host_end = src.find('/', host_start);
    if (host_end == std::string::npos)
      return false;
    dst.host = src.substr(host_start, host_end - host_start);
    std::size_t port_pos = dst.host.find(':');
    if (port_pos != std::string::npos) {
      dst.port = dst.host.substr(port_pos + 1);
      dst.host = dst.host.substr(0, port_pos);
    }
  }

  // absolute path or the path part of absolute URI
  std::size_t path_start =
      (scheme_pos == std::string::npos) ? 0 : src.find('/', scheme_pos + 3);
  std::size_t path_end = src.find('?', path_start);
  if (path_end == std::string::npos)
    path_end = src.find('#', path_start);
  if (path_end == std::string::npos)
    path_end = src.size();
  dst.path = src.substr(path_start, path_end - path_start);

  // query
  std::size_t query_start = src.find('?', path_end);
  if (query_start != std::string::npos) {
    std::size_t query_end = src.find('#', query_start);
    if (query_end == std::string::npos)
      query_end = src.size();
    dst.query = src.substr(query_start + 1, query_end - query_start - 1);
  }

  // fragment
  std::size_t fragment_start = src.find('#', path_end);
  if (fragment_start != std::string::npos) {
    dst.fragment = src.substr(fragment_start + 1);
  }
  return true;
}

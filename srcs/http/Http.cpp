#include "Http.hpp"

bool Http::SplitURI(URI &dst, const std::string &src) {
  std::size_t scheme_pos = src.find("://");
  if (scheme_pos != std::string::npos) {
    // absolute URI
    dst.scheme = src.substr(0, scheme_pos);
    std::size_t host_start = scheme_pos + 3;
    std::size_t host_end = src.find('/', host_start);
    if (host_end == std::string::npos)
      return false;

    std::string authority = src.substr(host_start, host_end - host_start);
    std::size_t userinfo_pos = authority.find('@');
    if (userinfo_pos != std::string::npos) {
      std::string userinfo = authority.substr(0, userinfo_pos);
      std::size_t pass_pos = userinfo.find(':');
      if (pass_pos != std::string::npos) {
        dst.user = userinfo.substr(0, pass_pos);
        dst.pass = userinfo.substr(pass_pos + 1);
      } else {
        dst.user = userinfo;
      }
      authority = authority.substr(userinfo_pos + 1);
    }

    std::size_t port_pos = authority.find(':');
    if (port_pos != std::string::npos) {
      dst.port = authority.substr(port_pos + 1);
      dst.host = authority.substr(0, port_pos);
    } else {
      dst.host = authority;
    }
  } else {
    // relative path, all string considered path
    dst.path = src;
    return src[0] == '/';
  }

  // the path part of absolute URI
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

std::string Http::DeHexify(std::string uri) {
  std::string result;
  result.reserve(uri.size());

  for (std::size_t i = 0; i < uri.size(); ++i) {
    if (uri[i] == '%' && i + 2 < uri.size()) {
      if (!IsHexDigit(uri[i + 1]) || !IsHexDigit(uri[i + 2])) {
        result += uri[i]; // Plain characters.
        continue;
      } else {
        result +=
            static_cast<char>(16 * Hex2Char(uri[i + 1]) + Hex2Char(uri[i + 2]));
        i += 2;
      }
    } else {
      result += uri[i]; // Plain characters.
    }
  }

  return result;
}

bool Http::IsHexDigit(char c) {
  return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') ||
         ('A' <= c && c <= 'F');
}

int Http::Hex2Char(char c) {
  if ('0' <= c && c <= '9')
    return c - '0';
  else if ('a' <= c && c <= 'f')
    return c - 'a' + 10;
  else if ('A' <= c && c <= 'F')
    return c - 'A' + 10;
  else
    return 0;
}

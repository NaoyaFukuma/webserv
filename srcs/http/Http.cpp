#include "Http.hpp"
#include <iostream>

std::string Http::VersionToString(Version version) {
  switch (version) {
  case HTTP09:
    return "HTTP/0.9";
  case HTTP10:
    return "HTTP/1.0";
  case HTTP11:
    return "HTTP/1.1";
  default:
    std::cout << "VersionToString: unknown version " << version << std::endl;
    return "";
  }
}

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
    if (dst.path.empty()) {
      dst.path = '/';
    }
    if (src[0] != '/') {
      dst.path = '/' + dst.path;
    }
    // return true;
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
  if (dst.path.empty()) {
    dst.path = '/';
  }
  if (src[0] != '/') {
    dst.path = '/' + dst.path;
  }

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

std::string Http::DeHexify(std::string src) {
  std::string result;
  result.reserve(src.size());

  for (std::size_t i = 0; i < src.size(); ++i) {
    if (src[i] == '%' && i + 2 < src.size()) {
      if (!IsHexDigit(src[i + 1]) || !IsHexDigit(src[i + 2])) {
        result += src[i]; // Plain characters.
        continue;
      } else {
        result +=
            static_cast<char>(16 * Hex2Char(src[i + 1]) + Hex2Char(src[i + 2]));
        i += 2;
      }
    } else {
      result += src[i]; // Plain characters.
    }
  }

  return result;
}

// queryはcgiのためにデコードしない
void Http::DeHexify(Http::URI &uri) {
  uri.scheme = Http::DeHexify(uri.scheme);
  uri.user = Http::DeHexify(uri.user);
  uri.pass = Http::DeHexify(uri.pass);
  uri.host = Http::DeHexify(uri.host);
  uri.port = Http::DeHexify(uri.port);
  uri.path = Http::DeHexify(uri.path);
  uri.fragment = Http::DeHexify(uri.fragment);
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

Http::HttpStatus::HttpStatus() : status_code(-1), message(""){};

Http::HttpStatus::HttpStatus(int status_code, std::string message)
    : status_code(status_code), message(message){};

Http::HttpStatus::HttpStatus(int status_code)
    : status_code(status_code), message("") {
  std::map<int, std::string>::const_iterator it =
      kDefaultStatusMessage.find(status_code);
  if (it != kDefaultStatusMessage.end()) {
    message = it->second;
  }
};

const std::map<int, std::string> Http::HttpStatus::kDefaultStatusMessage =
    init_default_error_message();

std::map<int, std::string> Http::init_default_error_message() {
  std::map<int, std::string> m;
  m[100] = "Continue";
  m[101] = "Switching Protocols";
  m[102] = "Processing";  // WebDAV; RFC 2518
  m[103] = "Early Hints"; // RFC 8297

  m[200] = "OK";
  m[201] = "Created";
  m[202] = "Accepted";
  m[203] = "Non-Authoritative Information";
  m[204] = "No Content";
  m[205] = "Reset Content";
  m[206] = "Partial Content";
  m[207] = "Multi-Status";     // WebDAV; RFC 4918
  m[208] = "Already Reported"; // WebDAV; RFC 5842
  m[226] = "IM Used";          // RFC 3229

  m[300] = "Multiple Choices";
  m[301] = "Moved Permanently";
  m[302] = "Found";
  m[303] = "See Other";
  m[304] = "Not Modified";
  m[305] = "Use Proxy";
  m[306] = "Switch Proxy"; // Deprecated
  m[307] = "Temporary Redirect";
  m[308] = "Permanent Redirect"; // RFC 7538

  m[400] = "Bad Request";
  m[401] = "Unauthorized";
  m[402] = "Payment Required";
  m[403] = "Forbidden";
  m[404] = "Not Found";
  m[405] = "Method Not Allowed";
  m[406] = "Not Acceptable";
  m[407] = "Proxy Authentication Required";
  m[408] = "Request Timeout";
  m[409] = "Conflict";
  m[410] = "Gone";
  m[411] = "Length Required";
  m[412] = "Precondition Failed";
  m[413] = "Payload Too Large";
  m[414] = "URI Too Long";
  m[415] = "Unsupported Media Type";
  m[416] = "Range Not Satisfiable";
  m[417] = "Expectation Failed";
  m[418] = "I'm a teapot";         // April Fools' joke, RFC 2324
  m[421] = "Misdirected Request";  // RFC 7540
  m[422] = "Unprocessable Entity"; // WebDAV; RFC 4918
  m[423] = "Locked";               // WebDAV; RFC 4918
  m[424] = "Failed Dependency";    // WebDAV; RFC 4918
  m[425] = "Too Early";            // RFC 8470
  m[426] = "Upgrade Required";
  m[428] = "Precondition Required";           // RFC 6585
  m[429] = "Too Many Requests";               // RFC 6585
  m[431] = "Request Header Fields Too Large"; // RFC 6585
  m[451] = "Unavailable For Legal Reasons";   // RFC 7725

  m[500] = "Internal Server Error";
  m[501] = "Not Implemented";
  m[502] = "Bad Gateway";
  m[503] = "Service Unavailable";
  m[504] = "Gateway Timeout";
  m[505] = "HTTP Version Not Supported";
  m[506] = "Variant Also Negotiates";         // RFC 2295
  m[507] = "Insufficient Storage";            // WebDAV; RFC 4918
  m[508] = "Loop Detected";                   // WebDAV; RFC 5842
  m[510] = "Not Extended";                    // RFC 2774
  m[511] = "Network Authentication Required"; // RFC 6585
  return m;
}

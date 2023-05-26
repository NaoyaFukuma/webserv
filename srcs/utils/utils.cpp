#include "utils.hpp"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <vector>

ssize_t ws_split(std::vector<std::string> &dst, const std::string &src,
                 const char delim) {
  std::istringstream iss(src);
  if (!iss) {
    return -1;
  }
  std::vector<std::string> tokens;
  std::string token;

  while (std::getline(iss, token, delim)) {
    if (iss.fail() && !iss.eof()) {
      return -1;
    }
    tokens.push_back(token);
  }
  dst = tokens;

  return tokens.size();
}

bool ws_inet_addr(uint32_t &dst, std::string ip) {
  uint32_t res = 0;
  uint32_t tmp = 0;
  uint32_t cnt = 0;

  while (ip.find(".") != std::string::npos) {
    cnt++;
    if (cnt > 3 ||
        ws_strtoi<uint32_t>(&tmp, ip.substr(0, ip.find("."))) == false ||
        tmp > 255) {
      return false;
    }
    res = (res << 8) + tmp;
    ip = ip.substr(ip.find(".") + 1);
  }
  if (ws_strtoi<uint32_t>(&tmp, ip) == false || tmp > 255) {
    return false;
  }
  res = (res << 8) + tmp;
  dst = res;
  return true;
}

void *ws_memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s;
  while (n--) {
    *p++ = (unsigned char)c;
  }
  return s;
}

void *ws_memcpy(void *dest, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;
  while (n--) {
    *d++ = *s++;
  }
  return dest;
}

bool ws_isspace(const char c) {
  if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
      c == '\f') {
    return true;
  }
  return false;
}

bool end_with(const std::string &str, const std::string &query) {
  if (str.size() >= query.size() &&
      str.substr(str.size() - query.size()) == query) {
    return true;
  }
  return false;
}

FileType get_filetype(const std::string &path) {
  struct stat s;

  if (stat(path.c_str(), &s) == 0) {
    if (s.st_mode & S_IFDIR) {
      // it's a directory
      return FILE_DIRECTORY;
    } else if (s.st_mode & S_IFREG) {
      // it's a file
      return FILE_REGULAR;
    } else {
      // something else
      return FILE_OTHERS;
    }
  } else {
    // error or unknown
    return FILE_UNKNOWN;
  }
}

std::string get_date() {
  std::time_t now = std::time(nullptr);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&now), "%a, %d %b %Y %T GMT");
  return ss.str();
}

#include "utils.hpp"
#include <iostream>
#include <sstream>
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

#ifndef UTILS_HPP
#define UTILS_HPP

#include "utils.tpp"
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <vector>

ssize_t ws_split(std::vector<std::string> &dst, const std::string &src,
                 const char delim);
bool ws_inet_addr(uint32_t &dst, std::string ip);
void *ws_memset(void *s, int c, size_t n);
void *ws_memcpy(void *dest, const void *src, size_t n);
bool ws_isspace(const char c);

bool end_with(const std::string &str, const std::string &query);

enum filetype {
  FILE_UNKNOWN,
  FILE_REGULAR,
  FILE_DIRECTORY,
  FILE_OTHERS,
};

filetype get_filetype(const std::string &path);

#endif

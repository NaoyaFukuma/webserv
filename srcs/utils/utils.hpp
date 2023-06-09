#ifndef UTILS_HPP
#define UTILS_HPP

#include <ctime>
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <vector>

bool ws_exist_cgi_file(const std::string &path, const std::string &extension);
std::string ws_get_mime_type(const std::string &filename);
ssize_t ws_split(std::vector<std::string> &dst, const std::string &src,
                 const char delim);
bool ws_inet_addr(uint32_t &dst, std::string ip);
void *ws_memset(void *s, int c, std::size_t n);
void *ws_memcpy(void *dest, const void *src, std::size_t n);
bool ws_isspace(const char c);
template <typename T> bool ws_strtoi(T *dest, const std::string src);
template <typename T> std::string ws_itostr(const T src);
template <typename T> T mul_assert_overflow(T lhs, T rhs);

#include "utils.tpp"

bool end_with(const std::string &str, const std::string &query);

enum FileType {
  FILE_UNKNOWN,
  FILE_REGULAR,
  FILE_DIRECTORY,
  FILE_OTHERS,
};

FileType get_filetype(const std::string &path);
std::string get_date();
std::time_t str2time(std::string time_str);
std::string time2str(std::time_t time);

#endif

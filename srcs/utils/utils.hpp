#ifndef UTILS_HPP
#define UTILS_HPP

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
template <typename T> bool ws_strtoi(T *dest, const std::string src);
template <typename T> T mul_assert_overflow(T lhs, T rhs);

#include "utils.tpp"

bool end_with(const std::string &str, const std::string &query);

#endif

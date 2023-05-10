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
#endif
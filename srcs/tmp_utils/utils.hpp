#ifndef UTILS_HPP
#define UTILS_HPP

#include "utils.tpp"
#include <string>
#include <sys/types.h>
#include <vector>

ssize_t ms_split(std::vector<std::string> &dst, const std::string &src,
                 const char delim);

#endif
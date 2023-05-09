#include "utils.hpp"
#include <iostream>
#include <sstream>
#include <vector>

ssize_t ms_split(std::vector<std::string> &dst, const std::string &src,
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

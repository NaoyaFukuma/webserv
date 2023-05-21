#include "utils.hpp"
#include <limits>
#include <sstream>

// 正の整数のみ
template <typename T> bool ws_strtoi(T *dest, const std::string src) {
  T tmp;
  for (std::string::const_iterator it = src.begin(); it != src.end(); it++) {
    if (!isdigit(*it)) {
      return false;
    }
  }
  std::stringstream ss(src);
  ss >> tmp;
  if (ss.fail()) {
    return false;
  }
  if (tmp != 0 && src[0] == '0') {
    return false;
  }
  *dest = tmp;
  return true;
}

template <typename T> T mul_assert_overflow(T lhs, T rhs) {
  if (lhs > 0) {
    if (rhs > 0) {
      if (lhs > std::numeric_limits<T>::max() / rhs) {
        throw std::overflow_error("overflow in multiplication");
      }
    } else if (rhs < std::numeric_limits<T>::min() / lhs) {
      throw std::overflow_error("overflow in multiplication");
    }
  } else if (lhs < 0) {
    if (rhs > 0) {
      if (lhs < std::numeric_limits<T>::min() / rhs) {
        throw std::overflow_error("overflow in multiplication");
      }
    } else if (rhs < std::numeric_limits<T>::max() / lhs) {
      throw std::overflow_error("overflow in multiplication");
    }
  }
  return lhs * rhs;
}

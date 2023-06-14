#ifndef _DEFINE_HPP_
#define _DEFINE_HPP_
#include <cstdio>

#define FAILURE -1
#define SUCCESS 0

#ifndef DEBUG
#define DEBUG_PRINT(fmt, ...)                                                  \
  do {                                                                         \
  } while (0);
#else
#define DEBUG_PRINT(fmt, ...)                                                  \
  do {                                                                         \
    printf("func: %s, line: %d: ", __func__, __LINE__);                        \
    printf(fmt, ##__VA_ARGS__);                                                \
  } while (0);
#endif

#endif

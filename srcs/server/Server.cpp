#include "Epoll.hpp"
#include "define.hpp"
#include <sys/epoll.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: ./Server [config_file]" << std::endl;
    return FAILURE;
  }

  try {
    Config config;
    Epoll epoll;
    ParseConfig(config, argv[1]);
    std::cout << config << std::endl;
    epoll.RegisterListenSocket(config);
    epoll.EpollLoop();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return FAILURE;
  }
  return FAILURE;
}
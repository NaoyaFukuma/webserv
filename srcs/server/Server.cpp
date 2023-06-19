#include "Epoll.hpp"
#include "define.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  try {
    Config config;
    Epoll epoll;
    switch (argc) {
    case 1:
      config.ParseConfig(DEFAULT_CONFIG_FILE);
      break;
    case 2:
      config.ParseConfig(argv[1]);
      break;
    default:
      std::cerr << "Usage: \"./webserv\"  \"./webserv [config_file]\""
                << std::endl;
      return FAILURE;
    }
#ifdef DEBUG
    std::cerr << config << std::endl;
#endif
    epoll.RegisterListenSocket(config);
    epoll.EventLoop();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return FAILURE;
  }
  return FAILURE;
}

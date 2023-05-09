#include "Config.hpp"
#include "Epoll.hpp"
#include "Socket.hpp"
#include "define.hpp"
#include <sys/epoll.h>

void RegisterListenSocket(Epoll &epoll, const Config &config) {
  ConfigMap config_map = ConfigToMap(config);
  uint32_t epoll_mask = EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLOUT | EPOLLET;

  if (epoll.Create() == FAILURE) {
    throw std::runtime_error("Failed to create epoll");
  }
  for (ConfigMap::iterator it = config_map.begin(); it != config_map.end();
       it++) {
    std::cout << it->first << std::endl;
    ListenSocket *socket = new ListenSocket(it->second);
    if (socket->Create() == FAILURE || socket->Passive() == FAILURE ||
        epoll.Add(socket, epoll_mask) == FAILURE) {
      delete socket;
      throw std::runtime_error("Failed to register listen socket");
    }
  }
}

void ServerLoop(Epoll &epoll) {
  struct epoll_event events[MAX_EVENTS];
  while (true) {
    epoll.CheckTimeout();
    int nfds = epoll.Wait(events, MAX_EVENTS);
    if (nfds == -1) {
      throw std::runtime_error("Failed to epoll_wait");
    }
    for (int i = 0; i < nfds; i++) {
      int event_fd = events[i].data.fd;
      ASocket *event_socket = epoll.GetSocket(event_fd);
      if (event_socket == NULL) {
        continue;
      }
      uint32_t event_mask = events[i].events;
      event_socket->ProcessSocket(&epoll, (void *)&event_mask);
    }
  }
}

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
    RegisterListenSocket(epoll, config);
    ServerLoop(epoll);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return FAILURE;
  }
  return FAILURE;
}
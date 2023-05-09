#ifndef EPOLL_HPP_
#define EPOLL_HPP_

#include "Socket.hpp"
#include <map>

class ASocket; // 相互参照

class Epoll {
private:
  int epoll_fd_;
  std::map<int, ASocket *> fd_to_socket_;
  static const int epoll_timeout_ = 1000;
  static const int socket_timeout_ = 10;

public:
  Epoll();
  Epoll(const Epoll &src);
  ~Epoll();
  Epoll &operator=(const Epoll &rhs);

  ASocket *GetSocket(int fd);
  int Create();
  int Add(ASocket *socket, uint32_t event_mask);
  int Del(int fd);
  int Wait(struct epoll_event *events, int maxevents);
  void CheckTimeout();
};

#endif // EPOLL_HPP_
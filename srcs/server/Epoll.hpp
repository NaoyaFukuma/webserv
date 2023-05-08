#ifndef _EPOLL_HPP_
#define _EPOLL_HPP_

#include "Socket.hpp"
#include <map>

class ASocket; // 相互参照

class Epoll {
private:
  int epoll_fd_;
  std::map<int, ASocket *> fd_to_socket_;

public:
  Epoll();
  Epoll(const Epoll &src);
  ~Epoll();
  Epoll &operator=(const Epoll &rhs);

  ASocket *GetSocket(int fd);
  int Create();
  int Add(ASocket *socket, uint32_t event_mask);
  int Del(int fd);
  int Wait(struct epoll_event *events, int maxevents, int timeout);
};

#endif
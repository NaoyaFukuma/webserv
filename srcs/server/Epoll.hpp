#ifndef EPOLL_HPP_
#define EPOLL_HPP_

#include "Config.hpp"
#include "Socket.hpp"
#include <map>

class ASocket; // 相互参照

class Epoll {
private:
  int epoll_fd_;
  std::map<int, ASocket *> fd_to_socket_;
  static const int epoll_timeout_ = 100;
  static const int socket_timeout_ = 10;
  static const int max_events_ = 10;

  void CheckTimeout();

public:
  Epoll();
  ~Epoll();

  ASocket *GetSocket(int fd);
  void Add(ASocket *socket, uint32_t event_mask);
  void Del(int fd);
  void Mod(int fd, uint32_t event_mask);
  void RegisterListenSocket(const Config &config);
  void EventLoop();

private: // 使用予定なし
  Epoll(const Epoll &src);
  Epoll &operator=(const Epoll &rhs);
};

#endif // EPOLL_HPP_
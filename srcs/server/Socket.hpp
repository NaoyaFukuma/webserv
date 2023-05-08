#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include "SocketBuff.hpp"
#include "time.h"
#include <netinet/in.h>
#include <vector>

class Epoll; // 相互参照

// ------------------------------------------------------------------
// 継承用のクラス

class ASocket {
protected:
  int fd_;
  std::vector<Vserver> config_;
  time_t last_event_time_; // ListenSocketの場合は負の数に設定する

public:
  ASocket(std::vector<Vserver> config);
  ASocket(const ASocket &src);
  virtual ~ASocket();
  ASocket &operator=(const ASocket &rhs);

  int GetFd() const;
  void SetFd(int fd);
  int SetNonBlocking() const;
  virtual int ProcessSocket(Epoll *epoll_map, void *data) = 0;
};

// ------------------------------------------------------------------
// 通信用のソケット

class ConnSocket : public ASocket {
private:
  SocketBuff recv_buffer_;
  SocketBuff send_buffer_;
  std::deque<Request> requests_;

public:
  ConnSocket(std::vector<Vserver> config);
  ConnSocket(const ConnSocket &src);
  ~ConnSocket();
  ConnSocket &operator=(const ConnSocket &rhs);

  int OnReadable();
  int OnWritable();
  int ProcessSocket(Epoll *epoll_map, void *data);
};

// ------------------------------------------------------------------
// listen用のソケット

class ListenSocket : public ASocket {
private:
public:
  ListenSocket(std::vector<Vserver> config);
  ListenSocket(const ListenSocket &src);
  ~ListenSocket();
  ListenSocket &operator=(const ListenSocket &rhs);

  int Create();
  int Passive();
  ConnSocket *Accept();
  int ProcessSocket(Epoll *epoll_map, void *data);
};

#endif // SOCKET_HPP_
#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include "Config.hpp"
#include "SocketBuff.hpp"
#include <deque>
#include <netinet/in.h>
#include <time.h>
#include <vector>

class Epoll; // 相互参照

struct LastEventTime {
  time_t in_time;
  time_t out_time;
};

// ------------------------------------------------------------------
// 継承用のクラス

class ASocket {
protected:
  int fd_;
  std::vector<Vserver> config_;
  LastEventTime last_event_; // ListenSocketの場合は負の数に設定する

public:
  ASocket(std::vector<Vserver> config);
  ASocket(const ASocket &src);
  virtual ~ASocket();
  ASocket &operator=(const ASocket &rhs);

  int GetFd() const;
  void SetFd(int fd);
  int SetNonBlocking() const;
  bool IsTimeout(const time_t &threshold) const;
  virtual int ProcessSocket(Epoll *epoll_map, void *data) = 0;
};

// ------------------------------------------------------------------
// 通信用のソケット

class ConnSocket : public ASocket {
private:
  SocketBuff recv_buffer_;
  SocketBuff send_buffer_;
  bool rdhup_; // RDHUPが発生したかどうか
  // std::deque<Request> requests_;

public:
  ConnSocket(std::vector<Vserver> config);
  ~ConnSocket();

  int OnReadable(Epoll *epoll_map);
  int OnWritable(Epoll *epoll_map);
  int ProcessSocket(Epoll *epoll_map, void *data);

private: // 使用予定なし
  ConnSocket(const ConnSocket &src);
  ConnSocket &operator=(const ConnSocket &rhs);
};

// ------------------------------------------------------------------
// listen用のソケット

class ListenSocket : public ASocket {
public:
  ListenSocket(std::vector<Vserver> config);
  ~ListenSocket();

  int Create();
  int Passive();
  ConnSocket *Accept();
  int ProcessSocket(Epoll *epoll_map, void *data);

private: // 使用予定なし
  ListenSocket(const ListenSocket &src);
  ListenSocket &operator=(const ListenSocket &rhs);
};

#endif // SOCKET_HPP_
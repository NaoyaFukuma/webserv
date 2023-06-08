#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "SocketBuff.hpp"
#include <deque>
#include <netinet/in.h>
#include <time.h>
#include <vector>

class Epoll;     // 相互参照
class CgiSocket; // 相互参照

struct LastEventTime {
  std::time_t in_time;
  std::time_t out_time;
};

// ------------------------------------------------------------------
// 継承用のクラス
//    -> ConnScoket, ListenSocket, CgiSocketの３つのクラスへ継承する

class ASocket {
protected:
  int fd_;
  ConfVec config_;
  LastEventTime last_event_; // ListenSocketの場合は負の数に設定する
  Epoll *epoll_;

public:
  ASocket(ConfVec config, Epoll *epoll);
  virtual ~ASocket();

  ConfVec GetConfVec() const;
  int GetFd() const;
  Epoll *GetEpoll() const { return epoll_; }
  void SetFd(int fd);
  bool IsTimeout(const std::time_t &threshold) const;
  virtual int ProcessSocket(Epoll *epoll, void *data) = 0;

private: // 使用予定なし
  ASocket(const ASocket &src);
  ASocket &operator=(const ASocket &rhs);
};

// ------------------------------------------------------------------
// 通信用のソケット

class ConnSocket : public ASocket {
private:
  SocketBuff recv_buffer_;
  SocketBuff send_buffer_;
  bool rdhup_; // RDHUPが発生したかどうか
  std::deque<Request> requests_;
  std::deque<Response> responses_;
  std::pair<std::string, std::string> ip_port_;
  std::set<CgiSocket *> cgi_sockets_;

public:
  ConnSocket(ConfVec config, Epoll *epoll);
  ~ConnSocket();

  int OnReadable(Epoll *epoll);
  int OnWritable(Epoll *epoll);
  int ProcessSocket(Epoll *epoll, void *data);
  void AddResponse(const Response &response);
  void SetIpPort(const struct sockaddr_in &addr);
  void SetCgiSocket(CgiSocket *cgi_socket) { cgi_sockets_.insert(cgi_socket); }
  void DeleteCgiSocket(CgiSocket *cgi_socket) {
    cgi_sockets_.erase(cgi_socket);
  }
  void PushResponse(Response response);
  std::pair<std::string, std::string> GetIpPort() const;

private: // 使用予定なし
  ConnSocket(const ConnSocket &src);
  ConnSocket &operator=(const ConnSocket &rhs);
};

// ------------------------------------------------------------------
// listen用のソケット

class ListenSocket : public ASocket {
public:
  ListenSocket(ConfVec config, Epoll *epoll);
  ~ListenSocket();

  void Create();
  void Passive();
  ConnSocket *Accept();
  int ProcessSocket(Epoll *epoll, void *data);

private: // 使用予定なし
  ListenSocket(const ListenSocket &src);
  ListenSocket &operator=(const ListenSocket &rhs);
};

#endif // SOCKET_HPP_

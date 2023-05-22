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
  virtual ~ASocket();

  int GetFd() const;
  void SetFd(int fd);
  bool IsTimeout(const time_t &threshold) const;
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
  // std::deque<Request> requests_;

public:
  ConnSocket(std::vector<Vserver> config);
  ~ConnSocket();

  int OnReadable(Epoll *epoll);
  int OnWritable(Epoll *epoll);
  int ProcessSocket(Epoll *epoll, void *data);

private: // 使用予定なし
  ConnSocket(const ConnSocket &src);
  ConnSocket &operator=(const ConnSocket &rhs);
};

// ------------------------------------------------------------------
// CGI用のソケット

/*
HTTPクライアント
HTTPサーバー CGIクライアント
CGIスクリプト CGIサーバー
*/

class CgiSocket : public ASocket {
private:
  SocketBuff recv_buffer_;
  SocketBuff send_buffer_;
  bool rdhup_;
  pid_t pid_;               // CGIスクリプト実行用のプロセスID
  ConnSocket *conn_socket_; // CGI実行要求したHTTPクライアント

public:
  CgiSocket(ConnSocket *conn_socket);
  ~CgiSocket();
  CgiSocket *CreatCgiProcess();
  int OnWritable(Epoll *epoll);
  int OnReadable(Epoll *epoll);
  int ProcessSocket(Epoll *epoll, void *data);

private: // 使用予定なし
  CgiSocket(const CgiSocket &src);
  CgiSocket &operator=(const CgiSocket &rhs);

  // 内部でしか使わないメソッド
  void SetSocket(int child_sock, int parent_sock);
  char **SetMetaVariables();
};

// ------------------------------------------------------------------
// listen用のソケット

class ListenSocket : public ASocket {
public:
  ListenSocket(std::vector<Vserver> config);
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

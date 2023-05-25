#ifndef CGISOCKET_HPP_
#define CGISOCKET_HPP_

#include "Socket.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "SocketBuff.hpp"
#include <deque>
#include <netinet/in.h>
#include <time.h>
#include <vector>

class Epoll; // 相互参照

// struct LastEventTime {
//   time_t in_time;
//   time_t out_time;
// };


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
  pid_t pid_;               // CGIスクリプト実行用のプロセスID
  ConnSocket *conn_socket_; // CGI実行要求したHTTPクライアント
  Request http_request_;    // CGI実行要求したHTTPリクエスト

// ----------------  in CgiSocket.cpp  ------------------
public:
  // CGI実行要求したクライアント、そのHTTPリクエスト情報
  CgiSocket(ConnSocket *conn_socket, Request http_request);
  ~CgiSocket();
  // EPOLLイベントのハンドラー
  int ProcessSocket(Epoll *epoll, void *data);
  // EPOLLOUTに対応
  int OnWritable(Epoll *epoll);
  // EPOLLINに対応
  int OnReadable(Epoll *epoll);

// ----------------  in CreatCgiProcess.cpp  ------------------
public:
  CgiSocket *CreatCgiProcess();
private:
  void SetSocket(int child_sock, int parent_sock);
  char **SetMetaVariables();
  char **SetArgv();
  void SetCurrentDir(const std::string &cgi_path);


private: // 使用予定なし
  CgiSocket(const CgiSocket &src);
  CgiSocket &operator=(const CgiSocket &rhs);

};



#endif // CGISOCKET_HPP_

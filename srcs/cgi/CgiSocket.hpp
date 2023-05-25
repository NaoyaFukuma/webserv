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


/*
struct RequestLine {
  std::string method; // そのまま
  std::string uri; // 更新
  Http::Version version; // そのまま
};

typedef std::map<std::string, std::vector<std::string> > Header;

struct RequestMessage {
  RequestLine request_line; // 一部更新
  Header header; // 更新
  std::string body; // 更新
};

enum ParseStatus {
  INIT,
  HEADER,
  BODY,
  COMPLETE,
  ERROR,
};

struct ResourcePath {
  Http::URI uri; // 更新
  std::string server_path; // 更新
  std::string path_info; // 更新
};

struct Context {
  Vserver vserver; // そのまま
  std::string server_name; // そのまま
  Location location; // そのまま
  ResourcePath resource_path; // 更新
  bool is_cgi; // 更新
};
*/

struct CgiRequest
{
  RequestMessage message_;
  Context context_;
};

class CgiSocket : public ASocket {
private:
  SocketBuff recv_buffer_;
  SocketBuff send_buffer_;
  pid_t pid_;               // CGIスクリプト実行用のプロセスID
  ConnSocket *conn_socket_; // CGI実行要求したHTTPクライアント
  CgiRequest cgi_request_;  // HTTPリクエストから抜粋した
                            //CGIスクリプトに渡すリクエスト情報

// ----------------  in CgiSocket.cpp  ------------------
public:
  // CGI実行要求したクライアント、そのHTTPリクエスト情報
  CgiSocket(ConnSocket *conn_socket, Request &http_request);
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
  Context &GetContext();
  RequestMessage &GetRequestMessage();
  SocketBuff &GetSendBuffer() {
    return send_buffer_;
  }
  SocketBuff &GetRecvBuffer() {
    return recv_buffer_;
  }

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

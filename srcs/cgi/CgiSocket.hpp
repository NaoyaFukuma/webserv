#ifndef CGISOCKET_HPP_
#define CGISOCKET_HPP_

#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Socket.hpp"
#include "SocketBuff.hpp"
#include <deque>
#include <netinet/in.h>
#include <time.h>
#include <vector>

class Epoll; // 相互参照

class CgiSocket : public ASocket {
private:
  pid_t cgi_pid_;
  int child_sock_fd_;
  int parent_sock_fd_;
  ConnSocket &http_client_sock_;
  const Request src_http_request_;
  Response &dest_http_response_;
  bool http_client_timeout_flag_;

  // 注意 http_response_のDONEで代用できるので注意
  bool created_http_res_flag_;

public:
  SocketBuff recv_buffer_;
  SocketBuff send_buffer_;
  CgiSocket(ConnSocket &http_client_sock, const Request http_request,
            Response &http_response);
  ~CgiSocket();
  int ProcessSocket(Epoll *epoll, void *data);
  int OnWritable(Epoll *epoll);
  int OnReadable(Epoll *epoll);

  CgiSocket *CreateCgiProcess();

  int GetChildSock() const { return child_sock_fd_; }
  void SetChildSock(int sock) { child_sock_fd_ = sock; }

  int GetParentSockFd() const { return parent_sock_fd_; }
  void SetParentSockFd(int sock) { parent_sock_fd_ = sock; }
  pid_t GetCgiPid() const { return cgi_pid_; }
  void SetCgiPid(pid_t pid) { cgi_pid_ = pid; }

  bool GetCreatedHttpResFlag() const { return created_http_res_flag_; }
  void SetCreatedHttpResFlag(bool flag) { created_http_res_flag_ = flag; }
  ConnSocket &GetHttpClientSock() { return http_client_sock_; }

  bool GetHttpClientTimeoutFlag() const { return http_client_timeout_flag_; }
  void SetHttpClientTimeoutFlag(bool flag) { http_client_timeout_flag_ = flag; }

private:
  int CreateUnixDomainSocketPair();
  void SetChildProcessSocket();
  char **SetChildProcessMetaVariables();
  char **SetChildProcessArgv();
  void SetChildProcessCurrentDir(const std::string &cgi_path);
  int ForkWrapper();
  void ExeCgiScript();
  int SetParentProcessSocket();

  void SetInternalErrorHttpResponse();

private: // 使用予定なし
  CgiSocket(const CgiSocket &src);
  CgiSocket &operator=(const CgiSocket &rhs);
};

#endif // CGISOCKET_HPP_

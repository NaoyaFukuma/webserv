#include "Socket.hpp"
#include "Epoll.hpp"
#include "define.hpp"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// ------------------------------------------------------------------
// 継承用のクラス

ASocket::ASocket(std::vector<Vserver> config) : fd_(-1), config_(config) {
  last_event_time_.last_epollin_time = -1;
  last_event_time_.last_epollout_time = -1;
}

ASocket::ASocket(const ASocket &src) : fd_(src.fd_) {}

ASocket::~ASocket() { close(fd_); }

ASocket &ASocket::operator=(const ASocket &rhs) {
  if (this != &rhs) {
    fd_ = rhs.fd_;
  }
  return *this;
}

int ASocket::GetFd() const { return fd_; }

void ASocket::SetFd(int fd) { fd_ = fd; }

int ASocket::SetNonBlocking() const {
  int flags = fcntl(fd_, F_GETFL, 0);
  if (flags == -1) {
    return FAILURE;
  }
  if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
    return FAILURE;
  }
  return SUCCESS;
}

bool ASocket::IsTimeout(const time_t &threshold) const {
  // listen socketの場合はtimeoutしない
  time_t now = time(NULL);
  if (last_event_time_.last_epollin_time == -1 &&
      last_event_time_.last_epollout_time == -1) {
    return false;
  }
  return (now - last_event_time_.last_epollin_time > threshold ||
          now - last_event_time_.last_epollout_time > threshold);
}

// ------------------------------------------------------------------
// 通信用のソケット

ConnSocket::ConnSocket(std::vector<Vserver> config) : ASocket(config) {
  time_t now = time(NULL);
  last_event_time_.last_epollin_time = now;
  last_event_time_.last_epollout_time = now;
}

ConnSocket::ConnSocket(const ConnSocket &src) : ASocket(src) {}

ConnSocket::~ConnSocket() {}

ConnSocket &ConnSocket::operator=(const ConnSocket &rhs) {
  if (this != &rhs) {
    fd_ = rhs.fd_;
  }
  return *this;
}

// 0: 引き続きsocketを利用 -1: socketを閉じる
int ConnSocket::OnReadable() {
  recv_buffer_.ReadSocket(fd_);
  std::cout << recv_buffer_.GetString() << std::endl;
  send_buffer_.AddString(recv_buffer_.GetString());
  recv_buffer_.ClearBuff();
  // request_.Parse(recv_buffer_);
  // if (request_.GetStatus() == COMPLETE || request_.GetStatus() == ERROR) {
  //   // Response response = ProcessRequest(request_, config_);
  //   // send_buffer_.AddString(response.GetString());
  //   std::cout << request_ << std::endl;
  //   request_.Clear();
  // }
  return SUCCESS;
}

// 0: 引き続きsocketを利用 -1: socketを閉じる
int ConnSocket::OnWritable() {
  send_buffer_.SendSocket(fd_);
  return SUCCESS;
}

int ConnSocket::ProcessSocket(Epoll *epoll_map, void *data) {
  // clientからの通信を処理
  uint32_t event_mask = *(static_cast<uint32_t *>(data));
  if (event_mask & EPOLLIN) {
    // 受信(Todo: OnReadable(0))
    if (OnReadable() == FAILURE) {
      epoll_map->Del(fd_);
      return FAILURE;
    }
  }
  if (event_mask & EPOLLPRI) {
    // 緊急メッセージ(Todo: OnReadable(MSG_OOB))
    if (OnReadable() == FAILURE) {
      epoll_map->Del(fd_);
      return FAILURE;
    }
  }
  if (event_mask & EPOLLOUT) {
    // 送信
    if (OnWritable() == FAILURE) {
      epoll_map->Del(fd_);
      return FAILURE;
    }
  }
  if (event_mask & EPOLLRDHUP) {
    // Todo: クライアントが切断 -> bufferの中身を全て送信してからsocketを閉じる
    shutdown(fd_, SHUT_RD);
    shutdown(fd_, SHUT_WR);
    epoll_map->Del(fd_);
  }
  if (event_mask & EPOLLERR || event_mask & EPOLLHUP) {
    // エラー
    epoll_map->Del(fd_);
  }
  return SUCCESS;
}

// ------------------------------------------------------------------
// listen用のソケット

ListenSocket::ListenSocket(std::vector<Vserver> config) : ASocket(config) {}

ListenSocket::ListenSocket(const ListenSocket &src) : ASocket(src) {}

ListenSocket::~ListenSocket() {}

ListenSocket &ListenSocket::operator=(const ListenSocket &rhs) {
  if (this != &rhs) {
    fd_ = rhs.fd_;
  }
  return *this;
}

int ListenSocket::Create() {
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ < 0) {
    return FAILURE;
  }
  return SetNonBlocking();
}

int ListenSocket::Passive() {
  struct sockaddr_in sockaddr;
  std::string ip = config_[0].listen_.listen_ip_;
  int port = config_[0].listen_.listen_port_;

  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());
  sockaddr.sin_port = htons(port);
  // Bind server socket to address
  if (bind(fd_, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
    // error handling
    return FAILURE;
  }
  if (listen(fd_, SOMAXCONN) == -1) {
    return FAILURE;
  }
  return SUCCESS;
}

ConnSocket *ListenSocket::Accept() {
  ConnSocket *conn_socket = new ConnSocket(config_);
  struct sockaddr_in client_addr;
  socklen_t addrlen = sizeof(client_addr);

  conn_socket->SetFd(accept(fd_, (struct sockaddr *)&client_addr, &addrlen));
  if (conn_socket->GetFd() < 0) {
    delete conn_socket;
    return NULL;
  }
  if (conn_socket->SetNonBlocking() == FAILURE) {
    delete conn_socket;
    return NULL;
  }
  return conn_socket;
}

int ListenSocket::ProcessSocket(Epoll *epoll_map, void *data) {
  // 接続要求を処理
  (void)data;
  static uint32_t epoll_mask =
      EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLOUT | EPOLLET;

  ConnSocket *client_socket = Accept();
  if (client_socket == NULL ||
      epoll_map->Add(client_socket, epoll_mask) == FAILURE) {
    delete client_socket;
    return FAILURE;
  }
  return SUCCESS;
}

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
  last_event_.in_time = -1;
  last_event_.out_time = -1;
}

ASocket::~ASocket() { close(fd_); }

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
  time_t now = time(NULL);
  // listen socketの場合はtimeoutしない
  if (last_event_.in_time == -1 && last_event_.out_time == -1) {
    return false;
  }

  // epolloutを登録していない間はin_timeのみを見る
  return (
      now - last_event_.in_time > threshold ||
      (last_event_.out_time != -1 && now - last_event_.out_time > threshold));
}

// ------------------------------------------------------------------
// 通信用のソケット

ConnSocket::ConnSocket(std::vector<Vserver> config)
    : ASocket(config), rdhup_(false) {
  last_event_.in_time = time(NULL);
  last_event_.out_time = -1;
}

ConnSocket::~ConnSocket() {}

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int ConnSocket::OnReadable(Epoll *epoll_map) {
  last_event_.in_time = time(NULL);
  if (recv_buffer_.ReadSocket(fd_) == FAILURE) {
    return FAILURE;
  }
  std::cout << "recv_buffer_: " << recv_buffer_.GetString() << std::endl;
  send_buffer_.AddString(recv_buffer_.GetString());
  recv_buffer_.ClearBuff();
  if (epoll_map->Mod(fd_, EPOLLIN | EPOLLOUT | EPOLLET) == FAILURE) {
    return FAILURE;
  }
  last_event_.out_time = time(NULL);
  return SUCCESS;

  // Todo: requestのparse
  // request_.Parse(recv_buffer_);
  // if (request_.GetStatus() == COMPLETE || request_.GetStatus() == ERROR) {
  //   // Response response = ProcessRequest(request_, config_);
  //   // send_buffer_.AddString(response.GetString());
  //   std::cout << request_ << std::endl;
  //   request_.Clear();
  // }
}

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int ConnSocket::OnWritable(Epoll *epoll_map) {
  if (send_buffer_.SendSocket(fd_)) {
    // 送信完了
    if (epoll_map->Mod(fd_, EPOLLIN | EPOLLET) == FAILURE) {
      return FAILURE;
    }
    // rdhupが立っていたら送信完了後にsocketを閉じる
    if (rdhup_) {
      shutdown(fd_, SHUT_WR);
      return FAILURE;
    }
    last_event_.out_time = -1;
  } else {
    // 送信中
    if (epoll_map->Mod(fd_, EPOLLIN | EPOLLOUT | EPOLLET) == FAILURE) {
      return FAILURE;
    }
    last_event_.out_time = time(NULL);
  }
  return SUCCESS;
}

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int ConnSocket::ProcessSocket(Epoll *epoll_map, void *data) {
  // clientからの通信を処理
  std::cout << "Socket: " << fd_ << std::endl;
  uint32_t event_mask = *(static_cast<uint32_t *>(data));
  if (event_mask & EPOLLRDHUP) {
    // Todo:クライアントが切断->bufferの中身を全て送信してからsocketを閉じる
    std::cout << "EPOLLRDHUP" << std::endl;
    shutdown(fd_, SHUT_RD);
    if (send_buffer_.GetString().size() == 0) {
      shutdown(fd_, SHUT_WR);
      return FAILURE;
    }
    rdhup_ = true;
  }
  if (event_mask & EPOLLERR || event_mask & EPOLLHUP) {
    std::cout << "EPOLLERR || EPOLLHUP" << std::endl;
    // エラー
    return FAILURE;
  }
  if (event_mask & EPOLLIN) {
    // 受信(Todo: OnReadable(0))
    std::cout << "EPOLLIN" << std::endl;
    if (OnReadable(epoll_map) == FAILURE) {
      return FAILURE;
    }
  }
  if (event_mask & EPOLLOUT) {
    // 送信
    std::cout << "EPOLLOUT" << std::endl;
    if (OnWritable(epoll_map) == FAILURE) {
      return FAILURE;
    }
  }
  // Todo: EPOLLPRIの検討
  // if (event_mask & EPOLLPRI) {
  //   // 緊急メッセージ(Todo: OnReadable(MSG_OOB))
  //   if (OnReadable() == FAILURE) {
  //     return FAILURE;
  //   }
  // }
  return SUCCESS;
}

// ------------------------------------------------------------------
// listen用のソケット

ListenSocket::ListenSocket(std::vector<Vserver> config) : ASocket(config) {}

ListenSocket::~ListenSocket() {}

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

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int ListenSocket::ProcessSocket(Epoll *epoll_map, void *data) {
  // 接続要求を処理
  (void)data;
  static const uint32_t epoll_mask = EPOLLIN | EPOLLET;

  ConnSocket *client_socket = Accept();
  if (client_socket == NULL ||
      epoll_map->Add(client_socket, epoll_mask) == FAILURE) {
    delete client_socket;
    return FAILURE;
  }
  return SUCCESS;
}

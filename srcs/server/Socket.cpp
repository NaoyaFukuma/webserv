#include "Socket.hpp"
#include "Epoll.hpp"
#include "define.hpp"
#include "utils.hpp"
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

ASocket::~ASocket() {
  if (close(fd_) < 0) {
    std::cerr << "Keep Running Error: close" << std::endl;
  }
}

int ASocket::GetFd() const { return fd_; }

void ASocket::SetFd(int fd) { fd_ = fd; }

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
int ConnSocket::OnReadable(Epoll *epoll) {
  last_event_.in_time = time(NULL);
  // ReadSocket == FAILURE: 相手がclose()かshutdown()した
  if (recv_buffer_.ReadSocket(fd_) == FAILURE) {
    rdhup_ = true;
  }

  while (recv_buffer_.FindString("\r\n\r\n") >= 0) {
    if (request_.empty() || request_.back().GetStatus() == COMPLETE ||
        request_.back().GetStatus() == ERROR) {
      request_.push_back(Request());
    }
    request_.back().Parse(recv_buffer_);
  }

  for (std::deque<Request>::iterator it = request_.begin();
       it != request_.end();) {
    if (it->GetStatus() == COMPLETE || it->GetStatus() == ERROR) {
      Response response = ProcessRequest(*it, config_);
      send_buffer_.AddString(response.GetString());
      epoll->Mod(fd_, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
      last_event_.out_time = time(NULL);
      std::deque<Request>::iterator tmp = it + 1;
      request_.erase(it);
      it = tmp;
    } else {
      it++;
    }
  }
  return SUCCESS;
}

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int ConnSocket::OnWritable(Epoll *epoll) {
  int send_result = send_buffer_.SendSocket(fd_);
  if (send_result < 0) {
    return FAILURE;
  } else if (send_result == 1) {
    // 送信完了
    epoll->Mod(fd_, EPOLLIN | EPOLLRDHUP | EPOLLET);
    // rdhupが立っていたら送信完了後にsocketを閉じる
    if (rdhup_) {
      if (shutdown(fd_, SHUT_WR) < 0) {
        std::cerr << "Keep Running Error: shutdown" << std::endl;
      }
      return FAILURE;
    }
    last_event_.out_time = -1;
  } else {
    // 送信中
    last_event_.out_time = time(NULL);
  }
  return SUCCESS;
}

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int ConnSocket::ProcessSocket(Epoll *epoll, void *data) {
  // clientからの通信を処理
  // std::cout << "Socket: " << fd_ << std::endl;
  uint32_t event_mask = *(static_cast<uint32_t *>(data));
  if (event_mask & EPOLLERR || event_mask & EPOLLHUP) {
    std::cout << fd_ << ": EPOLLERR || EPOLLHUP" << std::endl;
    // エラー
    return FAILURE;
  }
  if (event_mask & EPOLLIN) {
    // 受信(Todo: OnReadable(0))
    std::cout << fd_ << ": EPOLLIN" << std::endl;
    if (OnReadable(epoll) == FAILURE) {
      return FAILURE;
    }
  }
  if (event_mask & EPOLLOUT) {
    // 送信
    std::cout << fd_ << ": EPOLLOUT" << std::endl;
    if (OnWritable(epoll) == FAILURE) {
      return FAILURE;
    }
  }
  if (event_mask & EPOLLRDHUP) {
    // Todo:クライアントが切断->bufferの中身を全て送信してからsocketを閉じる
    std::cout << fd_ << ": EPOLLRDHUP" << std::endl;
    if (shutdown(fd_, SHUT_RD) < 0) {
      std::cerr << "Keep Running Error: shutdown" << std::endl;
    }
    if (send_buffer_.GetString().size() == 0) {
      if (shutdown(fd_, SHUT_WR) < 0) {
        std::cerr << "Keep Running Error: shutdown" << std::endl;
      }
      return FAILURE;
    }
    rdhup_ = true;
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

ListenSocket::ListenSocket(std::vector<Vserver> config) : ASocket(config) {
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ < 0) {
    throw std::runtime_error("Fatal Error: socket");
  }
  int yes = 1;
  if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
    throw std::runtime_error("Fatal Error: setsockopt");
  }
}

ListenSocket::~ListenSocket() {}

void ListenSocket::Passive() {
  struct sockaddr_in sockaddr;
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr = config_[0].listen_.sin_addr.s_addr;
  sockaddr.sin_port = config_[0].listen_.sin_port;
  // Bind server socket to address
  if (bind(fd_, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
    // error handling
    throw std::runtime_error("Fatal Error: bind");
  }
  if (listen(fd_, SOMAXCONN) == -1) {
    throw std::runtime_error("Fata Error: listen");
  }
}

ConnSocket *ListenSocket::Accept() {
  ConnSocket *conn_socket = new ConnSocket(config_);

  int fd = accept(fd_, NULL, NULL);
  if (fd < 0) {
    std::cerr << "Keep Running Error: accept" << std::endl;
    delete conn_socket;
    return NULL;
  }
  conn_socket->SetFd(fd);
  return conn_socket;
}

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int ListenSocket::ProcessSocket(Epoll *epoll, void *data) {
  // 接続要求を処理
  (void)data;
  static const uint32_t epoll_mask = EPOLLIN | EPOLLRDHUP | EPOLLET;

  ConnSocket *client_socket = Accept();
  if (client_socket == NULL) {
    return FAILURE;
  }
  epoll->Add(client_socket, epoll_mask);
  return SUCCESS;
}

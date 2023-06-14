#include "Socket.hpp"
#include "CgiSocket.hpp"
#include "Epoll.hpp"
#include "define.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

// ------------------------------------------------------------------
// 継承用のクラス

ASocket::ASocket(ConfVec config, Epoll *epoll)
    : fd_(-1), config_(config), epoll_(epoll) {
  last_event_.in_time = -1;
  last_event_.out_time = -1;
}

ASocket::~ASocket() {
  DEBUG_PRINT("%d: ASocket::~ASocket()\n", fd_);
  if (close(fd_) < 0) {
    std::cerr << "Keep Running Error: close" << std::endl;
  }
}

ConfVec ASocket::GetConfVec() const { return config_; }

int ASocket::GetFd() const { return fd_; }

void ASocket::SetFd(int fd) { fd_ = fd; }

bool ASocket::IsTimeout(const std::time_t &threshold) const {
  std::time_t now = time(NULL);
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

ConnSocket::ConnSocket(ConfVec config, Epoll *epoll)
    : ASocket(config, epoll), rdhup_(false) {
  last_event_.in_time = time(NULL);
  last_event_.out_time = -1;
}

ConnSocket::~ConnSocket() {
  DEBUG_PRINT("%d: ConnSocket::~ConnSocket()\n", fd_);

  for (std::set<CgiSocket *>::const_iterator it = cgi_sockets_.begin();
       it != cgi_sockets_.end();) {
        DEBUG_PRINT("%d: delete cgi_socket fd:%d\n", fd_, (*it)->GetParentSockFd());
    (*it)->SetHttpClientTimeoutFlag(true);
    GetEpoll()->Del((*it)->GetParentSockFd());
    cgi_sockets_.erase(it++);
  }
}

void ConnSocket::AddResponse(const Response &response) {
  responses_.push_back(response);
}

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int ConnSocket::OnReadable(Epoll *epoll) {
  last_event_.in_time = time(NULL);
  // ReadSocket == FAILURE: 相手がclose()かshutdown()した
  if (recv_buffer_.ReadSocket(fd_) == FAILURE) {
    rdhup_ = true;
  }

  while (recv_buffer_.FindString("\r\n") >= 0) {
    if (requests_.empty() || requests_.back().GetParseStatus() == COMPLETE ||
        requests_.back().GetParseStatus() == ERROR) {
      requests_.push_back(Request());
    }
    requests_.back().Parse(recv_buffer_, this);
  }

  for (std::deque<Request>::iterator it = requests_.begin();
       it != requests_.end();) {
    if (it->GetParseStatus() == COMPLETE || it->GetParseStatus() == ERROR) {
      responses_.push_back(Response());
      responses_.back().ProcessRequest(*it, this, epoll);
      it = requests_.erase(it);
    } else {
      it++;
    }
  }
  return SUCCESS;
}

// SUCCESS: 引き続きsocketを利用 FAILURE: socketを閉じる
int ConnSocket::OnWritable(Epoll *epoll) {
  for (std::deque<Response>::iterator it = responses_.begin();
       it != responses_.end() && !rdhup_;) {
    if (it->GetProcessStatus() == DONE) {
      DEBUG_PRINT("before: AddString\n%s\n", it->GetString().c_str());
      send_buffer_.AddString(it->GetString());
      rdhup_ = !it->GetIsConnection();
      it = responses_.erase(it);
    } else {
      it++;
    }
  }
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
  DEBUG_PRINT("Soket: %d\n", fd_);
  uint32_t event_mask = *(static_cast<uint32_t *>(data));
  if (event_mask & EPOLLERR || event_mask & EPOLLHUP) {
    DEBUG_PRINT("%d: EPOLLERR || EPOLLHUP\n", fd_);
    // エラー
    return FAILURE;
  }
  if (event_mask & EPOLLIN) {
    // 受信(Todo: OnReadable(0))
    DEBUG_PRINT("%d: EPOLLIN\n", fd_);
    if (OnReadable(epoll) == FAILURE) {
      return FAILURE;
    }
  }
  if (event_mask & EPOLLOUT) {
    // 送信
    DEBUG_PRINT("%d: EPOLLOUT\n", fd_);
    if (OnWritable(epoll) == FAILURE) {
      return FAILURE;
    }
  }
  if (event_mask & EPOLLRDHUP) {
    // Todo:クライアントが切断->bufferの中身を全て送信してからsocketを閉じる
    DEBUG_PRINT("%d: EPOLLRDHUP\n", fd_);
    if (shutdown(fd_, SHUT_RD) < 0) {
      std::cerr << "Keep Running Error: shutdown" << std::endl;
      return FAILURE;
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

void ConnSocket::SetIpPort(const struct sockaddr_in &addr) {
  std::ostringstream oss;

  // ipアドレスをntohl()を使用してネットワークバイトオーダからホストバイトオーダに変換
  oss << ((ntohl(addr.sin_addr.s_addr) >> 24) & 0xff) << "."
      << ((ntohl(addr.sin_addr.s_addr) >> 16) & 0xff) << "."
      << ((ntohl(addr.sin_addr.s_addr) >> 8) & 0xff) << "."
      << (ntohl(addr.sin_addr.s_addr) & 0xff);

  // ip_port_ のfirstにIPアドレス、secondにポート番号を格納
  ip_port_.first = oss.str();

  // 続いて、port番号を文字列にしてip_port_のsecondに格納
  oss.str(""); // 一旦リセット
  oss << ntohs(addr.sin_port);
  ip_port_.second = oss.str();
}

void ConnSocket::PushResponse(Response response) {
  responses_.push_back(response);
}

std::pair<std::string, std::string> ConnSocket::GetIpPort() const {
  return ip_port_;
}

// ------------------------------------------------------------------
// listen用のソケット

ListenSocket::ListenSocket(ConfVec config, Epoll *epoll)
    : ASocket(config, epoll) {
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ < 0) {
    throw std::runtime_error("Fatal Error: socket");
  }
  int yes = 1;
  if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
    throw std::runtime_error("Fatal Error: setsockopt");
  }
  DEBUG_PRINT("ListenSocket: %d\n", fd_);
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
  ConnSocket *conn_socket = new ConnSocket(config_, GetEpoll());

  struct sockaddr_in sockaddr_in_instance;
  socklen_t len = sizeof(sockaddr_in_instance);
  int fd = accept(
      fd_, reinterpret_cast<struct sockaddr *>(&sockaddr_in_instance), &len);
  DEBUG_PRINT("Accept: %d\n", fd);
  if (fd < 0) {
    std::cerr << "Keep Running Error: accept" << std::endl;
    delete conn_socket;
    return NULL;
  }
  conn_socket->SetIpPort(sockaddr_in_instance);

  int yes = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
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

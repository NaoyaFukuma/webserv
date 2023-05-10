#include "Epoll.hpp"
#include "define.hpp"
#include <sys/epoll.h>
#include <unistd.h>

Epoll::Epoll() : epoll_fd_(-1), fd_to_socket_() {
  epoll_fd_ = epoll_create(1);
  if (epoll_fd_ == -1) {
    throw std::runtime_error("Failed to create epoll");
  }
}

Epoll::~Epoll() {
  if (close(epoll_fd_) < 0) {
    std::cerr << "Systemcall Error: close" << std::endl;
  }

  std::map<int, ASocket *>::iterator it = fd_to_socket_.begin();
  while (it != fd_to_socket_.end()) {
    delete it->second;
    fd_to_socket_.erase(it++);
  }
}

ASocket *Epoll::GetSocket(int fd) {
  if (fd_to_socket_.find(fd) == fd_to_socket_.end())
    return NULL;
  return fd_to_socket_[fd];
}

void Epoll::Add(ASocket *socket, uint32_t event_mask) {
  struct epoll_event ev;
  int fd = socket->GetFd();

  ev.events = event_mask;
  ev.data.fd = fd;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
    throw std::runtime_error("Failed to epoll_ctl");
  }
  if (fd_to_socket_.find(fd) != fd_to_socket_.end()) {
    delete fd_to_socket_[fd];
  }
  fd_to_socket_[fd] = socket;
}

void Epoll::Del(int fd) {
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
    throw std::runtime_error("Failed to epoll_ctl");
  }
  if (fd_to_socket_.find(fd) != fd_to_socket_.end()) {
    delete fd_to_socket_[fd];
    fd_to_socket_.erase(fd);
  }
}

void Epoll::Mod(int fd, uint32_t event_mask) {
  struct epoll_event ev;

  ev.events = event_mask;
  ev.data.fd = fd;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
    throw std::runtime_error("Failed to epoll_ctl");
  }
}

void Epoll::CheckTimeout() {
  for (std::map<int, ASocket *>::iterator it = fd_to_socket_.begin();
       it != fd_to_socket_.end();) {
    if (it->second->IsTimeout(socket_timeout_)) {
      int fd = it->first;
      it++;
      Del(fd);
    } else {
      it++;
    }
  }
}

void Epoll::RegisterListenSocket(const Config &config) {
  ConfigMap config_map = ConfigToMap(config);
  static const uint32_t epoll_mask = EPOLLIN | EPOLLET;

  for (ConfigMap::iterator it = config_map.begin(); it != config_map.end();
       it++) {
    ListenSocket *socket = new ListenSocket(it->second);
    if (socket->Create() == FAILURE || socket->Passive() == FAILURE) {
      delete socket;
      throw std::runtime_error("Failed to set up listen socket");
    }
    Add(socket, epoll_mask);
  }
}

void Epoll::EventLoop() {
  struct epoll_event events[max_events_];
  while (true) {
    CheckTimeout();
    int nfds = epoll_wait(epoll_fd_, events, max_events_, epoll_timeout_);
    if (nfds == -1) {
      throw std::runtime_error("Failed to epoll_wait");
    }
    for (int i = 0; i < nfds; i++) {
      int event_fd = events[i].data.fd;
      uint32_t event_mask = events[i].events;
      ASocket *event_socket = GetSocket(event_fd);
      if (event_socket == NULL) {
        continue;
      }
      if (event_socket->ProcessSocket(this, (void *)&event_mask) == FAILURE) {
        Del(event_fd);
      }
    }
  }
}
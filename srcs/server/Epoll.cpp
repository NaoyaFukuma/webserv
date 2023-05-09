#include "Epoll.hpp"
#include "Socket.hpp"
#include "define.hpp"
#include <sys/epoll.h>
#include <unistd.h>

Epoll::Epoll() : epoll_fd_(-1), fd_to_socket_() {}

Epoll::Epoll(const Epoll &src) { *this = src; }

Epoll::~Epoll() {
  close(epoll_fd_);

  std::map<int, ASocket *>::iterator it = fd_to_socket_.begin();
  while (it != fd_to_socket_.end()) {
    delete it->second;
    fd_to_socket_.erase(it++);
  }
}

Epoll &Epoll::operator=(const Epoll &rhs) {
  if (this != &rhs) {
    epoll_fd_ = rhs.epoll_fd_;
    fd_to_socket_ = rhs.fd_to_socket_;
  }
  return *this;
}

ASocket *Epoll::GetSocket(int fd) {
  if (fd_to_socket_.find(fd) == fd_to_socket_.end())
    return NULL;
  return fd_to_socket_[fd];
}

int Epoll::Create() {
  epoll_fd_ = epoll_create(1);
  if (epoll_fd_ == -1) {
    return FAILURE;
  }
  return SUCCESS;
}

int Epoll::Add(ASocket *socket, uint32_t event_mask) {
  struct epoll_event ev;
  int fd = socket->GetFd();

  ev.events = event_mask;
  ev.data.fd = fd;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
    return FAILURE;
  }
  if (fd_to_socket_.find(fd) != fd_to_socket_.end()) {
    delete fd_to_socket_[fd];
  }
  fd_to_socket_[fd] = socket;
  return SUCCESS;
}

int Epoll::Del(int fd) {
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
    return FAILURE;
  }
  if (fd_to_socket_.find(fd) != fd_to_socket_.end()) {
    delete fd_to_socket_[fd];
    fd_to_socket_.erase(fd);
  }
  return SUCCESS;
}

int Epoll::Wait(struct epoll_event *events, int maxevents, int timeout) {
  return epoll_wait(epoll_fd_, events, maxevents, timeout);
}

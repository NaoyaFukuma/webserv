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

void Epoll::CheckTimeout() {
  for (std::map<int, ASocket *>::iterator it = fd_to_socket_.begin();
       it != fd_to_socket_.end();) {
    if (it->second->IsTimeout(socket_timeout_)) {
      std::cerr << "timeout: " << it->first << std::endl;
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
  uint32_t epoll_mask = EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLOUT | EPOLLET;

  for (ConfigMap::iterator it = config_map.begin(); it != config_map.end();
       it++) {
    std::cout << it->first << std::endl;
    ListenSocket *socket = new ListenSocket(it->second);
    if (socket->Create() == FAILURE || socket->Passive() == FAILURE ||
        Add(socket, epoll_mask) == FAILURE) {
      delete socket;
      throw std::runtime_error("Failed to register listen socket");
    }
  }
}

void Epoll::EpollLoop() {
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
      event_socket->ProcessSocket(this, (void *)&event_mask);
    }
  }
}

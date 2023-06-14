#include "define.hpp"
#include "SocketBuff.hpp"
#include <algorithm>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>

// FAILURE -1: socketを閉じる, SUCCESS 0: socketを閉じない
int SocketBuff::ReadSocket(int fd) {
  char buf[kBuffSize];
  ssize_t len;
  ssize_t total_len = 0;

  while (true) {
    len = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
    // recv()がエラーを返すか、リモートがコネクションを閉じた場合にループを抜ける
    if (len <= 0) {
      break;
    }
    buffer_.insert(buffer_.end(), buf, buf + len);
    total_len += len;
    if (len < static_cast<ssize_t>(sizeof(buf))) {
      break;
    }
  }
  // len == 0の時、リモートがコネクションを閉じたことを意味する
  return len != 0 ? SUCCESS : FAILURE;
}

bool SocketBuff::GetLine(std::string &line) {
  std::vector<char>::iterator it =
      std::find(buffer_.begin() + read_position_, buffer_.end(), '\n');
  if (it == buffer_.end()) {
    return false;
  }
  line.assign(buffer_.begin() + read_position_, it);
  read_position_ = std::distance(buffer_.begin(), it) + 1;
  return true;
}

void SocketBuff::ResetSeekg() { read_position_ = 0; }

void SocketBuff::ResetSeekp() {
  // 不要になった
}

ssize_t SocketBuff::FindString(const std::string &target) {
  std::vector<char>::iterator it =
      std::search(buffer_.begin() + read_position_, buffer_.end(),
                  target.begin(), target.end());
  if (it == buffer_.end()) {
    return -1;
  }
  return std::distance(buffer_.begin(), it);
}

std::string SocketBuff::GetAndErase(std::size_t pos) {
  std::string str(buffer_.begin(), buffer_.begin() + pos);
  buffer_.erase(buffer_.begin(), buffer_.begin() + pos);
  read_position_ -= pos;
  return str;
}

bool SocketBuff::GetUntilCRLF(std::string &line) {
  const char CRLF[] = {'\r', '\n'};
  std::vector<char>::iterator it = std::search(buffer_.begin() + read_position_,
                                               buffer_.end(), CRLF, CRLF + 2);
  if (it == buffer_.end()) {
    return false;
  }
  line.assign(buffer_.begin() + read_position_, it);
  read_position_ = std::distance(buffer_.begin(), it) + 2;
  return true;
}

std::size_t SocketBuff::GetReadSize() {
  return buffer_.size() - read_position_;
}

void SocketBuff::Erase() {
  buffer_.clear();
  read_position_ = 0;
}

void SocketBuff::Erase(std::size_t n) {
  buffer_.erase(buffer_.begin(), buffer_.begin() + n);
  read_position_ -= n;
}

void SocketBuff::AddString(const std::string &str) {
  buffer_.insert(buffer_.end(), str.begin(), str.end());
}

std::string SocketBuff::GetString() {
  return std::string(buffer_.begin() + read_position_, buffer_.end());
}

int SocketBuff::SendSocket(int fd) {
  ssize_t len = static_cast<ssize_t>(buffer_.size());
  ssize_t send_len = send(fd, &buffer_[0], len, MSG_DONTWAIT);
  if (send_len < 0) {
    return -1;
  }
  Erase(send_len);
  return send_len == len;
}

std::size_t SocketBuff::GetBuffSize() { return buffer_.size(); }

void SocketBuff::ClearBuff() {
  buffer_.clear();
  read_position_ = 0;
}

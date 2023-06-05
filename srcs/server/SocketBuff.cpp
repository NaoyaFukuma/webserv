#include "SocketBuff.hpp"

#include "define.hpp"
#include <fstream>
#include <ios>
#include <iostream>
#include <sys/socket.h>

SocketBuff::SocketBuff() {}  // デフォルトコンストラクタ
SocketBuff::~SocketBuff() {} // デストラクタ

// FAILURE -1: socketを閉じる, SUCCESS 0: socketを閉じない
int SocketBuff::ReadSocket(int fd) {
  char buf[kBuffSize];
  ssize_t len;

  while (true) {
    len = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
    // recv()がエラーを返すか、リモートがコネクションを閉じた場合にループを抜ける
    if (len <= 0) {
      break;
    }
    ss_ << std::string(buf, len);
    if (len < static_cast<ssize_t>(sizeof(buf))) {
      break;
    }
  }
  // len == 0の時、リモートがコネクションを閉じたことを意味する
  return len != 0 ? SUCCESS : FAILURE;
}

bool SocketBuff::GetLine(std::string &line) {
  std::getline(ss_, line);

  std::ios_base::iostate state = ss_.rdstate();
  if (state & std::ios_base::eofbit) {
    return false;
  } else if (state & std::ios_base::badbit) {
    throw std::runtime_error("SocketBuff::GetLine() failed");
  }
  return true;
}

void SocketBuff::ResetSeekg() { ss_.seekg(0); }

void SocketBuff::ResetSeekp() { ss_.seekp(0); }

ssize_t SocketBuff::FindString(const std::string &target) {
  ssize_t original_pos = static_cast<ssize_t>(ss_.tellg());
  ssize_t target_pos = -1;
  std::size_t target_index = 0;
  char ch;

  while (ss_.get(ch)) {
    if (ch == target[target_index]) {
      ++target_index;
      if (target_index == target.size()) {
        target_pos = static_cast<ssize_t>(ss_.tellg()) -
                     static_cast<ssize_t>(target.size());
        break;
      }
    } else {
      target_index = 0;
    }
  }

  // Reset the stream position to the original position
  ss_.clear();
  ss_.seekg(original_pos, std::ios_base::beg);

  return target_pos;
}

std::string SocketBuff::GetAndErase(const std::size_t pos) {
  std::string str = ss_.str().substr(0, pos);
  ss_.str(ss_.str().erase(0, pos));
  ss_.seekg(0, std::ios::beg);
  ss_.seekp(0, std::ios::beg);
  return str;
}

bool SocketBuff::GetUntilCRLF(std::string &line) {
  ssize_t pos = FindString("\r\n");
  if (pos == -1) {
    return false;
  }
  line = GetAndErase(pos);
  Erase(2); // "\r\n"を削除
  return true;
}

void SocketBuff::Erase() {
  ss_.str(ss_.str().erase(0, ss_.tellg()));
  ss_.seekg(0, std::ios::beg);
  ss_.seekp(0, std::ios::beg);
}

void SocketBuff::Erase(std::size_t n) {
  ss_.str(ss_.str().erase(0, n));
  ss_.seekg(0, std::ios::beg);
  ss_.seekp(0, std::ios::beg);
}

std::size_t SocketBuff::GetReadSize() {
  return ss_.tellg() == -1 ? 0 : static_cast<std::size_t>(ss_.tellg());
}

void SocketBuff::AddString(const std::string &str) { ss_ << str; }

std::string SocketBuff::GetString() { return ss_.str(); }

void SocketBuff::ClearBuff() {
  ss_.str("");
  ss_.seekg(0, std::ios::beg);
  ss_.seekp(0, std::ios::beg);
}

int SocketBuff::SendSocket(const int fd) {
  ssize_t len = static_cast<ssize_t>(ss_.str().size());
  ssize_t send_len = send(fd, ss_.str().c_str(), len, MSG_DONTWAIT);
  if (send_len < 0) {
    return -1;
  }
  Erase(send_len);
  return send_len == len;
}

std::size_t SocketBuff::GetBuffSize() { return ss_.str().size(); }

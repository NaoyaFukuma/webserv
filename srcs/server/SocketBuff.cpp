#include "SocketBuff.hpp"
#include "define.hpp"
#include <algorithm>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>

// FAILURE -1: socketを閉じる, SUCCESS 0: socketを閉じない
int SocketBuff::ReadSocket(int fd) {
  char buf[kBuffSize];
  ssize_t len;

  len = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
  if (len < 0) {    // エラー
    DEBUG_PRINT("recv() failed\n");
    return FAILURE;
  }
  if (len == 0) {   // FINパケットを受け取った
    DEBUG_PRINT("FINパケットを受け取った\n");
    return FAILURE;
  }

  // 受信したデータがあるパターンなので、バッファに追加
  buffer_.insert(buffer_.end(), buf, buf + len);
  return SUCCESS; // len > 0の時 ->
                  // SUCCESSでソケットを閉じない。次回以降の受信を待つ
}

bool SocketBuff::GetLine(std::string &line) {
  std::vector<char>::iterator it =
      std::find(buffer_.begin() + read_position_, buffer_.end(), '\n');
  if (it == buffer_.end()) {
    return false;
  }
  line.assign(buffer_.begin() + read_position_, it);
  read_position_ = std::distance(buffer_.begin(), it) + 1;
  DEBUG_PRINT("GetLine() read_position_:%zu\n", read_position_);
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
    DEBUG_PRINT("FindString() not found\n");
    return -1;
  }
  DEBUG_PRINT("FindString() found\n");
  return std::distance(buffer_.begin(), it);
}

std::string SocketBuff::GetAndErase(std::size_t pos) {
  std::string str(buffer_.begin() + read_position_,
                  buffer_.begin() + read_position_ + pos);
  buffer_.erase(buffer_.begin(), buffer_.begin() + read_position_ + pos);
  read_position_ -= pos;
  DEBUG_PRINT("GetAndErase() read_position_:%zu\n", read_position_);
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
  DEBUG_PRINT("GetUntilCRLF() read_position_:%zu\n", read_position_);
  return true;
}

std::size_t SocketBuff::GetReadSize() {
  return buffer_.size() - read_position_;
}

void SocketBuff::Erase() {
  buffer_.clear();
  read_position_ = 0;
  DEBUG_PRINT("Erase() read_position_:%zu\n", read_position_);
}

void SocketBuff::Erase(std::size_t n) {
  buffer_.erase(buffer_.begin(), buffer_.begin() + n);
  read_position_ -= n;
  DEBUG_PRINT("Erase(std::size_t n) read_position_:%zu\n", read_position_);
}

void SocketBuff::AddString(const std::string &str) {
  buffer_.insert(buffer_.end(), str.begin(), str.end());
}

std::string SocketBuff::GetString() {
  DEBUG_PRINT("buffer_.size() = %zu\n", buffer_.size());
  DEBUG_PRINT("read_position_ = %zu\n", read_position_);
  return std::string(buffer_.begin() + read_position_, buffer_.end());
}

int SocketBuff::SendSocket(int fd) {
  // 送信するデータがkBuffSizeを超えていたら、kBuffSizeにする
  // つまり送信するデータの上限を決めている
  size_t len = buffer_.size();
  len = kBuffSize < len ? kBuffSize : len;

  ssize_t send_len = send(fd, &buffer_[0], len, MSG_DONTWAIT | MSG_NOSIGNAL);
  if (send_len < 0) { // エラー RSTパケットを受け取った時
    return -1;
  }
  buffer_.erase(buffer_.begin(), buffer_.begin() + send_len);
  return buffer_.empty(); // 送信しきれない場合はfalseを返す
}

std::size_t SocketBuff::GetBuffSize() {
  return buffer_.size() - read_position_;
}

void SocketBuff::ClearBuff() {
  buffer_.clear();
  read_position_ = 0;
}

#ifndef SOCKETBUFF_HPP_
#define SOCKETBUFF_HPP_

#include <string>
#include <sys/types.h>
#include <vector>

class SocketBuff {
private:
  std::vector<char> buffer_;
  std::size_t read_position_;
  static const std::size_t kBuffSize = 1024;

public:
  SocketBuff() : buffer_(std::vector<char>()), read_position_(0) {}
  ~SocketBuff() {}

  int ReadSocket(int fd);
  int SendSocket(const int fd);

  bool GetUntilCRLF(std::string &line);
  std::string GetAndErase(const std::size_t pos);
  bool GetLine(std::string &line);
  ssize_t FindString(const std::string &target);

  void AddString(const std::string &str);

  std::string GetString();
  std::size_t GetBuffSize();
  std::size_t GetReadSize();

  void Erase();
  void Erase(const std::size_t n);
  void ClearBuff();

  void ResetSeekg();
  void ResetSeekp();

private:
  SocketBuff(const SocketBuff &);
  SocketBuff &operator=(const SocketBuff &);
};

#endif // SOCKETBUFF_HPP_

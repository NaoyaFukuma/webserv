/* client */
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define OK "\033[32mOK\033[0m"
#define NG "\033[31mNG\033[0m"
#define SUCCESS 0
#define FAILURE 1

#define MAX_TRY 10
#define BUFF_SIZE 1024

#define EXEC_TEST(proc)                                                        \
  {                                                                            \
    std::string ps(#proc);                                                     \
    std::cout << ps << ": ";                                                   \
    proc;                                                                      \
  }

using namespace std;

int connect_to_server(std::string host, std::string port) {
  struct addrinfo hints, *result, *rp;
  int client_fd;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int err = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
  if (err != 0) {
    std::cerr << "getaddrinfo: " << gai_strerror(err) << std::endl;
    return -1;
  }

  for (int i = 0; i < MAX_TRY; i++) {
    for (rp = result; rp != NULL; rp = rp->ai_next) {
      client_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (client_fd == -1) {
        continue;
      }
      if (connect(client_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
        return client_fd;
      }
      close(client_fd);
    }
    usleep(1000);
  }
  return -1;
}

int read_file(std::string &dst, std::string path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return FAILURE;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  dst = buffer.str();
  return SUCCESS;
}

std::string read_socket(int fd) {
  char buf[BUFF_SIZE];
  std::string response = "";
  while (true) {
    int len = recv(fd, buf, sizeof(buf), 0);
    if (len <= 0) {
      break;
    }
    response += std::string(buf, len);
  }
  return response;
}

std::string read_socket(int fd, size_t recv_size, std::string &read_buff) {
  char buf[BUFF_SIZE];
  int len = 0;
  while (read_buff.size() < recv_size) {
    int recv_ = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
    if (recv_ == -1) {
      continue;
    } else if (recv_ == 0) {
      if (read_buff.size() < recv_size) {
        std::string response = read_buff;
        read_buff = "";
        std::cerr << "recv == 0 before reading all data" << std::endl;
        return response;
      }
      break;
    }
    read_buff += std::string(buf, recv_);
    len += recv_;
  }
  std::string response = read_buff.substr(0, recv_size);
  read_buff = read_buff.substr(recv_size);
  return response;
}

int assert_equal(std::string expected, std::string actual) {
  if (expected == actual) {
    std::cout << OK << std::endl;
    return SUCCESS;
  } else {
    std::cout << NG << std::endl;
    std::cout << "Expected: " << std::endl;
    std::cout << expected << std::endl;
    std::cout << "Actual: " << std::endl;
    std::cout << actual << std::endl;
    return FAILURE;
  }
}

void print_diff(std::string expected, std::string actual) {
  std::cout << "Expected: " << std::endl;
  std::cout << expected << std::endl;
  std::cout << "Actual: " << std::endl;
  std::cout << actual << std::endl;
}

int test_basic(std::string host, std::string port, std::string path,
               bool shut_ = false) {
  int client_fd = connect_to_server(host, port);
  if (client_fd == -1) {
    std::cout << NG << std::endl;
    std::cerr << "Failed to connect to server" << std::endl;
    return FAILURE;
  }
  std::string request;
  if (read_file(request, path) == FAILURE) {
    std::cout << NG << std::endl;
    std::cerr << "Failed to read request" << std::endl;
    return FAILURE;
  }
  ssize_t len = send(client_fd, request.c_str(), request.size(), 0);
  if (shut_) {
    shutdown(client_fd, SHUT_WR);
  }
  std::string response = read_socket(client_fd);
  close(client_fd);

  return assert_equal(request, response);
}

int test_multiple_request(std::string host, std::string port, std::string path,
                          size_t send_, bool shut_ = false,
                          bool read_ = false) {
  int client_fd = connect_to_server(host, port);
  std::string read_buff = "";
  if (client_fd == -1) {
    std::cout << NG << std::endl;
    std::cerr << "Failed to connect to server" << std::endl;
    return FAILURE;
  }
  std::string request;
  if (read_file(request, path) == FAILURE) {
    std::cout << NG << std::endl;
    std::cerr << "Failed to read request" << std::endl;
    return FAILURE;
  }
  for (int i = 0; i < send_; i++) {
    ssize_t len = send(client_fd, request.c_str(), request.size(), 0);
    if (read_) {
      std::string response = read_socket(client_fd, request.size(), read_buff);
      if (response != request) {
        std::cout << NG << std::endl;
        // print_diff(request, response);
        return FAILURE;
      }
    }
  }
  if (shut_) {
    shutdown(client_fd, SHUT_WR);
  }
  if (read_) {
    close(client_fd);
    if (read_buff == "") {
      std::cout << OK << std::endl;
      return SUCCESS;
    } else {
      std::cout << NG << std::endl;
      // print_diff("", read_buff);
      return FAILURE;
    }
  } else {
    std::string response =
        read_socket(client_fd, request.size() * send_, read_buff);
    close(client_fd);
    std::string expected = "";
    for (int i = 0; i < send_; i++) {
      expected += request;
    }
    if (response == expected && read_buff == "") {
      std::cout << OK << std::endl;
      return SUCCESS;
    } else {
      std::cout << NG << std::endl;
      // print_diff(expected, response + read_buff);
      return FAILURE;
    }
  }
}

int test_multiple_client(std::string host, std::string port, std::string path,
                         size_t client_, bool shut_ = false) {
  std::vector<int> client_fds(client_);
  for (int i = 0; i < client_; i++) {
    client_fds[i] = connect_to_server(host, port);
    if (client_fds[i] == -1) {
      std::cout << NG << std::endl;
      std::cerr << "Failed to connect to server" << std::endl;
      return FAILURE;
    }
  }
  std::string request = "";
  if (read_file(request, path) == FAILURE) {
    std::cout << NG << std::endl;
    std::cerr << "Failed to read request" << std::endl;
    return FAILURE;
  }
  for (int i = 0; i < client_; i++) {
    ssize_t len = send(client_fds[i], request.c_str(), request.size(), 0);
    if (shut_) {
      shutdown(client_fds[i], SHUT_WR);
    }
  }
  for (int i = 0; i < client_; i++) {
    std::string response = "";
    std::string read_buff = "";
    response = read_socket(client_fds[i], request.size(), read_buff);
    close(client_fds[i]);
    if (response != request || read_buff != "") {
      std::cout << NG << std::endl;
      // print_diff(request, response);
      return FAILURE;
    }
  }
  std::cout << OK << std::endl;
  return SUCCESS;
}

int test1() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/404.txt";
  bool shut_ = false;
  return test_basic(host, port, path, shut_);
}

int test2() {
  std::string host = "webserv";
  std::string port = "8000";
  std::string path = "./request/404.txt";
  bool shut_ = false;
  return test_basic(host, port, path, shut_);
}

int test3() {
  std::string host = "webserv";
  std::string port = "9090";
  std::string path = "./request/404.txt";
  bool shut_ = false;
  return test_basic(host, port, path, shut_);
}

int test4() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len4096.txt";
  bool shut_ = false;
  return test_basic(host, port, path, shut_);
}

int test5() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len3072.txt";
  bool shut_ = false;
  return test_basic(host, port, path, shut_);
}

// test when shutdown client socket
int test6() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/404.txt";
  bool shut_ = true;
  return test_basic(host, port, path, shut_);
}

int test7() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len4096.txt";
  bool shut_ = true;
  return test_basic(host, port, path, shut_);
}

int test8() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len3072.txt";
  bool shut_ = true;
  return test_basic(host, port, path, shut_);
}

// test when client send many requests
#define SEND 100000
int test9() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len4096.txt";
  size_t send_ = SEND;
  bool shut_ = true;
  bool read_ = true;
  return test_multiple_request(host, port, path, send_, shut_, read_);
}

int test10() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len4096.txt";
  size_t send_ = SEND;
  bool shut_ = false;
  bool read_ = true;
  return test_multiple_request(host, port, path, send_, shut_, read_);
}

int test11() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len4096.txt";
  size_t send_ = SEND;
  bool shut_ = true;
  bool read_ = false;
  return test_multiple_request(host, port, path, send_, shut_, read_);
}

int test12() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len4096.txt";
  size_t send_ = SEND;
  bool shut_ = false;
  bool read_ = false;
  return test_multiple_request(host, port, path, send_, shut_, read_);
}

// test when many clients send requests
#define CLIENT 1000
int test13() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len4096.txt";
  size_t client_ = CLIENT;
  bool shut_ = true;
  return test_multiple_client(host, port, path, client_, shut_);
}

int test14() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len4096.txt";
  size_t client_ = CLIENT;
  bool shut_ = false;
  return test_multiple_client(host, port, path, client_, shut_);
}

int main() {
  EXEC_TEST(test1());
  EXEC_TEST(test2());
  EXEC_TEST(test3());
  EXEC_TEST(test4());
  EXEC_TEST(test5());
  EXEC_TEST(test6());
  EXEC_TEST(test7());
  EXEC_TEST(test8());
  EXEC_TEST(test9());
  EXEC_TEST(test10());
  EXEC_TEST(test11());
  EXEC_TEST(test12());
  EXEC_TEST(test13());
  EXEC_TEST(test14());
}
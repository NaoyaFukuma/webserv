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
    if (len < 0) {
      break;
    }
    response += std::string(buf, len);
    if (len < sizeof(buf)) {
      break;
    }
  }
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

int test(std::string host, std::string port, std::string path,
         bool shut = false) {
  int client_fd = connect_to_server(host, port);
  std::string request;
  if (client_fd == -1) {
    std::cout << NG << std::endl;
    std::cerr << "Failed to connect to server" << std::endl;
    return FAILURE;
  }
  if (read_file(request, path) == FAILURE) {
    std::cout << NG << std::endl;
    std::cerr << "Failed to read request" << std::endl;
    return FAILURE;
  }
  send(client_fd, request.c_str(), request.size(), 0);
  if (shut) {
    shutdown(client_fd, SHUT_WR);
  }
  std::string response = read_socket(client_fd);
  close(client_fd);
  return assert_equal(request, response);
}

int test1() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/404.txt";
  return test(host, port, path);
}

int test2() {
  std::string host = "webserv";
  std::string port = "8000";
  std::string path = "./request/404.txt";
  return test(host, port, path);
}

int test3() {
  std::string host = "webserv";
  std::string port = "9090";
  std::string path = "./request/404.txt";
  return test(host, port, path);
}

int test4() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len4096.txt";
  return test(host, port, path);
}

int test5() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len3072.txt";
  return test(host, port, path);
}

// test when shutdown client socket
int test6() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/404.txt";
  return test(host, port, path, true);
}

int test7() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len4096.txt";
  return test(host, port, path, true);
}

int test8() {
  std::string host = "webserv";
  std::string port = "8080";
  std::string path = "./request/len3072.txt";
  return test(host, port, path, true);
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
}
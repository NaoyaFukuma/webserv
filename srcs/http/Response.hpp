#ifndef RESPONSE_HPP_
#define RESPONSE_HPP_

#include "Config.hpp"
#include "Http.hpp"
#include "Request.hpp"
#include <map>
#include <string>

class ConnSocket;
class Epoll;

enum ProcessStatus {
  PROCESSING,
  DONE,
};

class Response {
private:
  ProcessStatus process_status_;
  Http::Version version_;
  int status_code_;
  std::string status_message_;
  std::map<std::string, std::string> header_;
  std::string body_;

public:
  Response();
  ~Response();
  Response(const Response &src);
  Response &operator=(const Response &src);

  std::string GetString();
  ProcessStatus GetProcessStatus() const;
  void ProcessRequest(Request &request, ConnSocket *socket, Epoll *epoll);
  void ProcessStatic(Request &request, ConnSocket *socket, Epoll *epoll);
};

#endif // RESPONSE_HPP_

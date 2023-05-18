#ifndef RESPONSE_HPP_
#define RESPONSE_HPP_

#include "Config.hpp"
#include "Http.hpp"
#include "Request.hpp"
#include <map>
#include <string>

class Response {
private:
  Http::Version version_;
  int status_code_;
  std::string status_message_;
  std::map<std::string, std::string> header_;
  std::string body_;

public:
  Response();
  ~Response();
  std::string GetString();

private: // 使用予定なし
  Response(const Response &src);
  Response &operator=(const Response &src);
};

Response ProcessRequest(Request request, Config config);

#endif // RESPONSE_HPP_

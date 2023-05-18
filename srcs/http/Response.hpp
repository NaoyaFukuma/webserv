#include <map>
#include <string>

struct HttpResponse {
  std::string ;
  int statusCode;
  std::string statusMessage;
  std::map<std::string, std::string> headers;
  std::string body;
};

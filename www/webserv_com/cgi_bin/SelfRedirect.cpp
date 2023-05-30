/*
CGIスクリプトを実装
CGIレスポンスがローカルリダイレクトで、ローカルリダイレクト先が自身を指すようにする
*/

#include <iostream>
#include <sstream>

int main() {
  std::stringstream ss;
  ss << "<html><head><title>LocalRedirect</title></head><body>\r\n";
  ss << "<h1>LocalRedirect</h1>\r\n";
  ss << "<p>Redirecting to <a href=\"SelfLoopCgi.cpp\">SelfLoopCgi.cpp</a></p>\r\n";
  ss << "</body></html>\r\n";

  std::cout << "Location: /cgi-bin/SelfRedirectCgi.cgi\r\n";
  std::cout << "Content-type: text/html\r\n";
  std::cout << "Content-length: " << ss.str().length() << "\r\n\r\n";
  std::cout << ss.str() << std::endl;
  return 0;
}

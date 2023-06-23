#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <time.h>

const static std::string kDataBasePath = "/app/www/webserv_com/db/session.db";

void ResBadRequest(std::string message) {
  std::stringstream ss;
  ss << "<html><head><title>Bad Request</title></head><body>\r\n";
  ss << "<h1>Bad Request</h1>\r\n";
  ss << "<p>" << message << "</p>\r\n";
  ss << "</body></html>\r\n";

  std::cout << "Status: 400 Bad Request\r\n";
  std::cout << "Content-Length: " << ss.str().length() << "\r\n";
  std::cout << "Content-type: text/html\r\n\r\n";
  std::cout << ss.str();
}

void ResOkRegistered(std::string session_id) {
  std::stringstream ss;
  ss << "<html><head><title>OK</title></head><body>\r\n";
  ss << "<h1>OK</h1>\r\n";
  ss << "<p>Your username is registered."<< "</p>\r\n";
  ss << "</body></html>\r\n";

	const size_t body_len = ss.str().length();

  std::cout << "Status: 200 OK\r\n";
  std::cout << "Content-type: text/html\r\n";
	std::cout << "Set-Cookie: session-id=" << session_id << "\r\n";
  std::cout << "Content-Length: " << body_len << "\r\n\r\n";
  std::cout << ss.str();
}

// POSTリクエストでユーザー名を受け取り、set-coockieヘッダーにsession-idを設定する
int main(int argc, char const *argv[], char const *envp[]) {
	const char *method = getenv("REQUEST_METHOD");
	
	if (method == NULL || std::string(method) != "POST") {
		ResBadRequest("This resource is only for POST method.");
		return 0;
	}

	// ユーザー名をデータベースに登録する
	std::ofstream ofs(kDataBasePath, std::ios::app);
	if (!ofs) {
		ResBadRequest("Failed to open database.");
		return 0;
	}

	// 現在時刻をsession-idとして設定する
	time_t now = time(NULL);
	std::string session_id = std::to_string(now);
	ofs << session_id << std::endl;
	ofs << std::cin.rdbuf() << std::endl;
	ofs.close();

	ResOkRegistered(session_id);
	return 0;
}

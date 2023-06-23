#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <time.h>
#include <vector>

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

void ResOkRegistered(std::string username) {
  std::stringstream ss;
  ss << "<html><head><title>OK</title></head><body>\r\n";
  ss << "<h1>OK</h1>\r\n";
  ss << "<p>Welcome back " << username << "</p>\r\n";
  ss << "</body></html>\r\n";

	const size_t body_len = ss.str().length();

  std::cout << "Status: 200 OK\r\n";
  std::cout << "Content-type: text/html\r\n";
  std::cout << "Content-Length: " << body_len << "\r\n\r\n";
  std::cout << ss.str();
}

std::vector<std::string> splitString(const std::string& inputString, const std::string& delimiter) {
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = 0;
	while ((end = inputString.find(delimiter, start)) != std::string::npos) {
			std::string token = inputString.substr(start, end - start);
			tokens.push_back(token);
			start = end + delimiter.length();
	}
	std::string lastToken = inputString.substr(start);
	tokens.push_back(lastToken);
	return tokens;
}

bool find_username(const std::string& session_id, std::string& dst_username) {
	std::ifstream ifs(kDataBasePath);
	if (ifs.fail()) {
		return false;
	}
	std::string line;
	while (getline(ifs, line)) {
		if (line == session_id) {
			getline(ifs, line);
			dst_username = line.substr(std::string("username=").length());
			return true;
		} else {
			getline(ifs, line);
		}
	}
	return false;
}

// POSTリクエストでユーザー名を受け取り、set-coockieヘッダーにsession-idを設定する
int main(int argc, char const *argv[], char const *envp[]) {
	const char *method = getenv("REQUEST_METHOD");
	
	if (method == NULL || std::string(method) != "GET") {
		ResBadRequest("This resource is only for GET method.");
		return 0;
	}

	// Cookieフィールドを出力する
	const char *cookie = getenv("HTTP_COOKIE");
	const std::vector<std::string> splited = splitString(std::string(cookie), "; ");
	for (const std::string& s : splited) {
		if (s.find("session-id=") != std::string::npos) {
			const std::string session_id = s.substr(std::string("session-id=").length());
			std::string username;
			if (find_username(session_id, username)) {
				ResOkRegistered(username);
				return 0;
			} else {
				break;
			}
		}
	}
	ResBadRequest("Session is invalid. Please login.");
	return 0;
}

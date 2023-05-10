#include "ConfigParser.hpp"
#include "utils.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdarg.h>

ConfigParser::ConfigParser(const char *filepath)
    : file_content_(LoadFile(filepath)), it_(file_content_.begin()) {}

ConfigParser::~ConfigParser() {}

std::string ConfigParser::LoadFile(const char *filepath) {
  std::string dest;
  std::ifstream ifs(filepath);
  std::stringstream buffer;
  if (!ifs || ifs.fail()) {
    throw ParserException("Failed to open config file: %s", filepath);
  }
  buffer << ifs.rdbuf();
  ifs.close();
  dest = buffer.str();
  return dest;
}

// parser
void ConfigParser::Parse(Config &config) {
  while (!this->IsEof()) {
    this->SkipSpaces();
    if (this->GetWord() != "server") {
      throw ParserException("Unexpected block");
    }
    this->ParseServer(config);
  }
}

void ConfigParser::ParseServer(Config &config) {
  Vserver server;
  server.listen_.listen_port_ = -1;

  this->SkipSpaces();
  this->Expect('{');
  while (!this->IsEof() && *it_ != '}') {
    this->SkipSpaces();
    std::string token = GetWord();
    if (token == "listen") {
      this->ParseListen(server);
    } else if (token == "server_name") {
      this->ParseServerName(server);
    } else if (token == "timeout") {
      this->ParseTimeOut(server);
    } else if (token == "location") {
      this->ParseLocation(server);
    } else {
      throw ParserException("Unexpected token: %s", token.c_str());
    }
    this->SkipSpaces();
  }
  this->Expect('}');
  this->AssertServer(server);
  config.AddServer(server);
}

void ConfigParser::ParseListen(Vserver &server) {
  this->SkipSpaces();
  std::string listen_str = GetWord();
  this->SkipSpaces();
  this->Expect(';');
  this->AssertListen(server.listen_, listen_str);
}

void ConfigParser::ParseServerName(Vserver &server) {
  std::vector<std::string> new_server_names;
  while (!this->IsEof() && *it_ != ';') {
    this->SkipSpaces();
    std::string server_name = GetWord();
    this->SkipSpaces();
    AssertServerName(server_name);
    new_server_names.push_back(server_name);
  }
  this->Expect(';');
  server.server_names_ = new_server_names;
}

void ConfigParser::ParseTimeOut(Vserver &server) {
  this->SkipSpaces();
  std::string timeout_str = GetWord();
  this->SkipSpaces();
  this->Expect(';');
  // TODO:
  server.timeout_ = (timeout_str);
}

void ConfigParser::ParseLocation(Vserver &server) {

  Location location;
  SetLocationDefault(location);

  this->SkipSpaces();
  location.path_ = GetWord();
  this->SkipSpaces();
  this->Expect('{');
  while (!this->IsEof() && *it_ != '}') {
    this->SkipSpaces();
    std::string token = GetWord();
    if (token == "match") {
      ParseMatch(location);
    } else if (token == "allow_method") {
      ParseAllowMethod(location);
    } else if (token == "max_body_size") {
      ParseMaxBodySize(location);
    } else if (token == "root") {
      ParseRoot(location);
    } else if (token == "index") {
      ParseIndex(location);
    } else if (token == "is_cgi") {
      ParseIsCgi(location);
    } else if (token == "cgi_path") {
      ParseCgiPath(location);
    } else if (token == "error_page") {
      ParseErrorPages(location);
    } else if (token == "autoindex") {
      ParseAutoIndex(location);
    } else if (token == "return") {
      ParseReturn(location);
    } else {
      throw ParserException("Unexpected token: %s", token.c_str());
    }
    this->SkipSpaces();
  }
  this->Expect('}');
  AssertLocation(location);
  server.locations_.push_back(location);
}

void ConfigParser::SetLocationDefault(Location &location) {
  location.match_ = PREFIX;
  location.max_body_size_ = 1024 * 1024; // 1MB
  location.is_cgi_ = false;
  location.autoindex_ = false;
  location.return_.first = -1;
}

void ConfigParser::ParseMatch(Location &location) {
  this->SkipSpaces();
  std::string match_str = GetWord();
  this->SkipSpaces();
  this->Expect(';');
  AssertMatch(location.match_, match_str);
}

void ConfigParser::ParseAllowMethod(Location &location) {
  if (!location.allow_method_.empty()) {
    throw ParserException("allow_method is already set");
  };
  while (!this->IsEof() && *it_ != ';') {
    this->SkipSpaces();
    std::string method_str = GetWord();
    this->SkipSpaces();
    AssertAllowMethod(location.allow_method_, method_str);
  }
  this->Expect(';');
}

void ConfigParser::ParseMaxBodySize(Location &location) {
  this->SkipSpaces();
  std::string size_str = GetWord();
  this->SkipSpaces();
  this->Expect(';');
  AssertMaxBodySize(location.max_body_size_, size_str);
}

void ConfigParser::ParseRoot(Location &location) {
  this->SkipSpaces();
  location.root_ = GetWord();
  this->SkipSpaces();
  this->Expect(';');
  AssertRoot(location.root_);
}

void ConfigParser::ParseIndex(Location &location) {
  while (!this->IsEof() && *it_ != ';') {
    this->SkipSpaces();
    std::string index_str = GetWord();
    this->SkipSpaces();
    AssertIndex(location.index_, index_str);
  }
  this->Expect(';');
}

void ConfigParser::ParseIsCgi(Location &location) {
  this->SkipSpaces();
  std::string is_cgi_str = GetWord();
  this->SkipSpaces();
  this->Expect(';');
  AssertBool(location.is_cgi_, is_cgi_str);
}

void ConfigParser::ParseCgiPath(Location &location) {
  this->SkipSpaces();
  location.cgi_path_ = GetWord();
  this->SkipSpaces();
  this->Expect(';');
  AssertCgiPath(location.cgi_path_);
}

void ConfigParser::ParseErrorPages(Location &location) {
  while (!this->IsEof() && *it_ != ';') {
    std::vector<int> error_codes;
    std::string error_page_str;
    while (true) {
      int error_code;
      this->SkipSpaces();
      error_page_str = GetWord();
      this->SkipSpaces();
      if (!ws_strtoi(&error_code, error_page_str)) {
        break;
      }
      error_codes.push_back(error_code);
    }
    AssertErrorPages(location.error_pages_, error_codes, error_page_str);
  }
  this->Expect(';');
}

void ConfigParser::ParseAutoIndex(Location &location) {
  this->SkipSpaces();
  std::string autoindex_str = GetWord();
  this->SkipSpaces();
  this->Expect(';');
  AssertBool(location.autoindex_, autoindex_str);
}

void ConfigParser::ParseReturn(Location &location) {
  std::string return_code_str;
  std::string return_path_str;

  this->SkipSpaces();
  return_code_str = GetWord();
  this->SkipSpaces();
  if (IsEof()) {
    throw ParserException("unexpected EOF");
  }
  if (*it_ != ';') {
    return_path_str = GetWord();
    this->SkipSpaces();
  }
  this->Expect(';');
  AssertReturn(location.return_, return_code_str, return_path_str);
}

// validator
void ConfigParser::AssertServer(const Vserver &server) {
  if (server.listen_.listen_port_ == -1) {
    throw ParserException("Listen port is not set");
  }
}

void ConfigParser::AssertListen(struct sockaddr_in &dest_listen,
                                const std::string &listen_str) {

  std::string host_str;
  std::string port_str;

  size_t colon_pos = listen_str.find(':');
  if (colon_pos == std::string::npos) {
    host_str = "0.0.0.0";
    port_str = listen_str;
  } else {
    host_str = listen_str.substr(0, colon_pos); // ドメイン名 or IPアドレス
    port_str = listen_str.substr(colon_pos + 1);
  }

  // getaddrinfo()を使って ドメイン名 or IPアドレス 及びポート番号のチェック
  struct addrinfo hints, *res;
  int err;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_INET;
  if ((err = getaddrinfo(host_str.c_str(), port_str.c_str(), &hints, &res)) !=
      0) {
    throw ParserException("getaddrinfo: " + std::string(gai_strerror(err)));
  }
  if (res->ai_family != AF_INET) {
    throw ParserException("getaddrinfo: not AF_INET");
  }
  memcpy(&dest_listen, res->ai_addr, sizeof(struct sockaddr_in));
  freeaddrinfo(res);
  return;
}

bool ConfigParser::IsValidIp(const std::string &ip_str) {
  std::string::const_iterator it_start = ip_str.begin();
  std::string::const_iterator it_end = ip_str.begin();

  std::string octet_str;
  int num_octets = 0;
  int octet;

  while (true) {
    while (it_end != ip_str.end() && *it_end != '.') {
      it_end++;
    }
    octet_str = std::string(it_start, it_end);
    if (++num_octets > 4) {
      return false;
    }
    if (!ws_strtoi<int>(&octet, octet_str) || octet < 0 || 255 < octet) {
      return false;
    }
    if (it_end == ip_str.end()) {
      break;
    }
    it_start = it_end + 1;
    it_end = it_start;
  }
  return num_octets == 4;
}

void ConfigParser::AssertServerName(const std::string &server_name) {
  if (server_name.empty()) {
    throw ParserException("Empty server name");
  }
  if (server_name.size() > kMaxDomainLength) {
    throw ParserException("Invalid server name: %s", server_name.c_str());
  }
  for (std::string::const_iterator it = server_name.begin();
       it < server_name.end();) {
    if (!IsValidLabel(server_name, it)) {
      throw ParserException("Invalid server name: %s", server_name.c_str());
    }
  }
}

// 各ラベルの長さが1~63文字であり、ハイフンと英数字のみを含む(先頭、末尾はハイフン以外)ことを確認
bool ConfigParser::IsValidLabel(const std::string &server_name,
                                std::string::const_iterator &it) {
  std::string::const_iterator start = it;
  while (it < server_name.end() && *it != '.') {
    if (!(isdigit(*it) || isalpha(*it) || *it == '-')) {
      return false;
    }
    if (*it == '-' && (it == start || it + 1 == server_name.end())) {
      return false;
    }
    it++;
  }
  if (it - start == 0 || it - start > kMaxDomainLabelLength) {
    return false;
  }
  if (it < server_name.end() && *it == '.') {
    it++;
    if (it == server_name.end()) {
      return false;
    }
  }
  return true;
}

void ConfigParser::AssertLocation(const Location &location) { (void)location; }

void ConfigParser::AssertMatch(match_type &dest_match,
                               const std::string &match_str) {
  if (match_str == "prefix") {
    dest_match = PREFIX;
  } else if (match_str == "back") {
    dest_match = BACK;
  } else {
    throw ParserException("Invalid match type: %s", match_str.c_str());
  }
}

void ConfigParser::AssertAllowMethod(std::set<method_type> &dest_method,
                                     const std::string &method_str) {
  method_type src_method;

  if (method_str == "GET") {
    src_method = GET;
  } else if (method_str == "POST") {
    src_method = POST;
  } else if (method_str == "DELETE") {
    src_method = DELETE;
  } else {
    throw ParserException("Invalid method: %s", method_str.c_str());
  }
  if (dest_method.find(src_method) != dest_method.end()) {
    throw ParserException("Duplicated method: %s", method_str.c_str());
  }
  dest_method.insert(src_method);
}

void ConfigParser::AssertMaxBodySize(uint64_t &dest_size,
                                     const std::string &size_str) {
  char unit = 'B';
  std::size_t len = size_str.length();

  if (len == 0) {
    throw ParserException("Empty size str");
  }
  if (size_str[len - 1] == 'B' || size_str[len - 1] == 'K' ||
      size_str[len - 1] == 'M' || size_str[len - 1] == 'G') {
    unit = size_str[len - 1];
    len--;
  }
  if (len == 0) {
    throw ParserException("Invalid size: %s", size_str.c_str());
  }
  if (!ws_strtoi<uint64_t>(&dest_size, size_str.substr(0, len))) {
    throw ParserException("Invalid size: %s", size_str.c_str());
  }
  switch (unit) {
  case 'B':
    break;
  case 'K':
    dest_size = mul_assert_overflow<uint64_t>(dest_size, 1024);
    break;
  case 'M':
    dest_size = mul_assert_overflow<uint64_t>(dest_size, 1024 * 1024);
    break;
  case 'G':
    dest_size = mul_assert_overflow<uint64_t>(dest_size, 1024 * 1024 * 1024);
    break;
  default:
    throw ParserException("Invalid size: %s", size_str.c_str());
  }
}

void ConfigParser::AssertRoot(const std::string &root) {
  // AssertPath(root);
  (void)root;
}

void ConfigParser::AssertIndex(std::vector<std::string> &dest_index,
                               const std::string &index_str) {
  dest_index.push_back(index_str);
}

void ConfigParser::AssertBool(bool &dest_bool, const std::string &bool_str) {
  if (bool_str == "on") {
    dest_bool = true;
  } else if (bool_str == "off") {
    dest_bool = false;
  } else {
    throw ParserException("Invalid cgi: %s", bool_str.c_str());
  }
}

void ConfigParser::AssertCgiPath(const std::string &cgi_path) {
  // AssertPath(cgi_path);
  (void)cgi_path;
}

void ConfigParser::AssertErrorPages(
    std::map<int, std::string> &dest_error_pages,
    const std::vector<int> error_codes, const std::string &error_page_str) {
  static const int valid_codes = 39;
  static const int init_list[valid_codes] = {
      400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412,
      413, 414, 415, 416, 417, 418, 421, 422, 423, 424, 426, 428, 429,
      431, 451, 500, 501, 502, 503, 504, 505, 506, 507, 508, 510, 511};
  static const std::set<int> valid_error_status_codes(init_list,
                                                      init_list + valid_codes);

  if (error_codes.empty()) {
    throw ParserException("Empty error code");
  }
  for (std::vector<int>::const_iterator it = error_codes.begin();
       it != error_codes.end(); it++) {
    if (valid_error_status_codes.count(*it) == 0) {
      throw ParserException("Invalid error code: %d", *it);
    }
    dest_error_pages[*it] = error_page_str;
  }
}

void ConfigParser::AssertReturn(std::pair<int, std::string> &dest_return,
                                const std::string &return_code_str,
                                const std::string &return_path_str) {
  int return_code;
  static const int valid_codes = 57;
  static const int init_list[valid_codes] = {
      200, 201, 202, 203, 204, 205, 206, 207, 208, 226, 300, 301, 302, 303, 304,
      305, 307, 308, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411,
      412, 413, 414, 415, 416, 417, 418, 421, 422, 423, 424, 426, 428, 429, 431,
      451, 500, 501, 502, 503, 504, 505, 506, 507, 508, 510, 511};
  static const std::set<int> valid_status_codes(init_list,
                                                init_list + valid_codes);

  if (!ws_strtoi<int>(&return_code, return_code_str)) {
    throw ParserException("Invalid return code: %s", return_code_str.c_str());
  }
  if (valid_status_codes.count(return_code) == 0) {
    throw ParserException("Invalid return code: %d", return_code);
  }
  // return ディレクティブは上書きではなく、最初の設定を優先
  if (dest_return.first == -1) {
    dest_return.first = return_code;
    dest_return.second = return_path_str;
  }
}

// utils
char ConfigParser::GetC() {
  if (IsEof()) {
    throw ParserException("Unexpected EOF");
  }
  return *it_++;
}

void ConfigParser::Expect(const char c) {
  if (IsEof()) {
    throw ParserException("Unexpected EOF");
  }
  if (*it_ != c) {
    throw ParserException("Expected %c, but unexpected char: %c", c, *it_);
  }
  it_++;
}

void ConfigParser::SkipSpaces() {
  while (!this->IsEof() && isspace(*it_)) {
    it_++;
  }
}

std::string ConfigParser::GetWord() {
  std::string word;
  while (!this->IsEof() && !this->IsDelim() && !isspace(*it_)) {
    word += *it_++;
  }
  if (word.empty()) {
    throw ParserException("Empty Word");
  }
  return word;
}

bool ConfigParser::IsEof() { return it_ == file_content_.end(); }

bool ConfigParser::IsDelim() {
  return *it_ == ';' || *it_ == '{' || *it_ == '}';
}

// ParserException
ConfigParser::ParserException::ParserException(const char *errfmt, ...) {
  va_list args;
  va_start(args, errfmt);

  vsnprintf(errmsg_, MAX_ERROR_LEN, errfmt, args);
  va_end(args);
}

const char *ConfigParser::ParserException::what() const throw() {
  return errmsg_;
}

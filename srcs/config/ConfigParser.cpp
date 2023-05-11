#include "ConfigParser.hpp"
#include <fstream>
#include <iostream>
#include <netdb.h> // for getaddrinfo()
#include <netinet/in.h>
#include <sstream>
#include <stdarg.h>

// 行、列、行の内容、エラーの理由を出力する
#define ERR_MSG_ROW_COL_LINE "Config Error: row %d, col %d\n%s <--- %s\n"

// エラー理由を出力する
#define ERR_MSG "Config Error: %s\n"

// 正の整数のみ
template <typename T> bool ws_strtoi(T *dest, const std::string src) {
  T tmp;
  for (std::string::const_iterator it = src.begin(); it != src.end(); it++) {
    if (!isdigit(*it)) {
      return false;
    }
  }
  std::stringstream ss(src);
  ss >> tmp;
  if (ss.fail()) {
    return false;
  }
  if (tmp != 0 && src[0] == '0') {
    return false;
  }
  *dest = tmp;
  return true;
}

template <typename T> T mul_assert_overflow(T lhs, T rhs) {
  if (lhs > 0) {
    if (rhs > 0) {
      if (lhs > std::numeric_limits<T>::max() / rhs) {
        throw std::overflow_error("overflow in multiplication");
      }
    } else if (rhs < std::numeric_limits<T>::min() / lhs) {
      throw std::overflow_error("overflow in multiplication");
    }
  } else if (lhs < 0) {
    if (rhs > 0) {
      if (lhs < std::numeric_limits<T>::min() / rhs) {
        throw std::overflow_error("overflow in multiplication");
      }
    } else if (rhs < std::numeric_limits<T>::max() / lhs) {
      throw std::overflow_error("overflow in multiplication");
    }
  }
  return lhs * rhs;
}

ConfigParser::ConfigParser(const char *filepath)
    : file_content_(LoadFile(filepath)), it_(file_content_.begin()) {
  std::cout << file_content_ << std::endl; // debug
}

ConfigParser::~ConfigParser() {}

std::string ConfigParser::LoadFile(const char *filepath) {
  std::string dest;
  std::ifstream ifs(filepath);
  std::stringstream buffer;
  if (!ifs || ifs.fail()) {
    throw ParserException("Config Error: open config file: %s", filepath);
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
      int row;
      int col;
      std::string line;
      this->GetErrorPoint(row, col, line);
      throw ParserException(ERR_MSG_ROW_COL_LINE, row, col, line.c_str(),
                            "Expected 'server'");
    }
    this->ParseServer(config);
    this->SkipSpaces();
  }
  AssertConfig(config);
}

void ConfigParser::ParseServer(Config &config) {
  Vserver server;

  server.timeout_ = 60;                // default timeout
  server.listen_.sin_family = AF_INET; // IPv4 default
  server.listen_.sin_port = htons(80); // default port

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
      int row;
      int col;
      std::string line;
      this->GetErrorPoint(row, col, line);
      throw ParserException(ERR_MSG_ROW_COL_LINE, row, col, line.c_str(),
                            "Unexpected token");
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
  AssertTimeOut(server.timeout_, timeout_str);
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
    } else if (token == "client_max_body_size") {
      ParseClientMaxBodySize(location);
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
      int row;
      int col;
      std::string line;
      this->GetErrorPoint(row, col, line);
      throw ParserException(ERR_MSG_ROW_COL_LINE, row, col, line.c_str(),
                            "Unexpected token");
    }
    this->SkipSpaces();
  }
  this->Expect('}');
  AssertLocation(location);
  server.locations_.push_back(location);
}

void ConfigParser::SetLocationDefault(Location &location) {
  location.match_ = PREFIX;
  location.allow_methods_.insert(GET);
  location.allow_methods_.insert(POST);
  location.allow_methods_.insert(DELETE);
  location.client_max_body_size_ = 1024 * 1024; // 1MB
  location.autoindex_ = false;
  location.is_cgi_ = false;
  location.return_.return_type_ = RETURN_EMPTY;
}

void ConfigParser::ParseMatch(Location &location) {
  this->SkipSpaces();
  std::string match_str = GetWord();
  this->SkipSpaces();
  this->Expect(';');
  AssertMatch(location.match_, match_str);
}

void ConfigParser::ParseAllowMethod(Location &location) {
  location.allow_methods_.clear();
  while (!this->IsEof() && *it_ != ';') {
    this->SkipSpaces();
    std::string method_str = GetWord();
    this->SkipSpaces();
    AssertAllowMethod(location.allow_methods_, method_str);
  }
  this->Expect(';');
  AssertAllowMethods(location.allow_methods_);
}

void ConfigParser::ParseClientMaxBodySize(Location &location) {
  this->SkipSpaces();
  std::string size_str = GetWord();
  this->SkipSpaces();
  this->Expect(';');
  AssertClientMaxBodySize(location.client_max_body_size_, size_str);
}

void ConfigParser::ParseRoot(Location &location) {
  this->SkipSpaces();
  location.root_ = GetWord();
  this->SkipSpaces();
  this->Expect(';');
  AssertRoot(location.root_);
}

void ConfigParser::ParseIndex(Location &location) {

  this->SkipSpaces();
  location.index_ = GetWord();
  this->SkipSpaces();
  AssertIndex(location.index_);
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
  // return ディレクティブは、複数指定可能だが、その場合最初の return
  // ディレクティブのみ有効
  if (location.return_.return_type_ != RETURN_EMPTY) {
    while (*it_++ != ';') { // すべての return ディレクティブを読み飛ばす
    }
    return;
  }

  this->SkipSpaces();
  std::string tmp_str = GetWord();

  // １つ目のワードが、status_codeかURLorTextかを判定
  if (ws_strtoi(&location.return_.status_code_, tmp_str)) {
    // status codeが設定されたので、URLかtextかを判定
    // textはシングルクォーテーションで囲まれている
    // URLはシングルクォーテーションで囲まれていない
    this->SkipSpaces();
    if (*it_ == ';') { // status codeのみ指定された場合
      location.return_.return_type_ = RETURN_ONLY_STATUS_CODE;
    } else {
      tmp_str = GetWord();
      if (tmp_str[0] == '\'') {
        // text
        location.return_.return_type_ = RETURN_TEXT;
        location.return_.return_text_ = tmp_str.substr(1, tmp_str.size() - 2);
        location.return_.text_length_ = location.return_.return_text_.size();
      } else {
        // URL
        location.return_.return_type_ = RETURN_URL;
        location.return_.return_url_ = tmp_str;
      }
    }
  } else { // status codeがない場合はURLとみなす
    location.return_.status_code_ = 302; // defaultはリダレクト
    location.return_.return_type_ = RETURN_URL;
    location.return_.return_url_ = tmp_str;
  }
  this->SkipSpaces();
  this->Expect(';');
  AssertReturn(location.return_);
}

// validator
void ConfigParser::AssertConfig(const Config &config) {
  if (config.GetServerVec().empty()) {
    throw ParserException(ERR_MSG, "server brock is not set");
  }
}

void ConfigParser::AssertServer(const Vserver &server) {
  if (server.server_names_.empty()) {
    throw ParserException(ERR_MSG, "server name is not set");
  }
  if (server.locations_.empty()) {
    throw ParserException(ERR_MSG, "location is not set");
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
    throw ParserException(ERR_MSG, gai_strerror(err));
  }
  if (res->ai_family != AF_INET) {
    throw ParserException(ERR_MSG, "not AF_INET");
  }
  memcpy(&dest_listen, res->ai_addr, sizeof(struct sockaddr_in));
  freeaddrinfo(res);
  return;
}

void ConfigParser::AssertServerName(const std::string &server_name) {
  if (server_name.empty()) {
    throw ParserException(ERR_MSG, "Empty server name");
  }
  if (server_name.size() > kMaxDomainLength) {
    throw ParserException(
        ERR_MSG, (server_name + " is Invalid too long server name").c_str());
  }
  for (std::string::const_iterator it = server_name.begin();
       it < server_name.end();) {
    if (!IsValidLabel(server_name, it)) {
      throw ParserException(
          ERR_MSG, (server_name + " is Invalid labal server name").c_str());
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

void ConfigParser::AssertTimeOut(int &dest_timeout,
                                 const std::string &timeout_str) {
  if (!ws_strtoi<int>(&dest_timeout, timeout_str)) {
    throw ParserException(ERR_MSG,
                          (timeout_str + " is Invalid timeout").c_str());
  }
}

void ConfigParser::AssertLocation(const Location &location) {
  // root ディレクティブが無いとエラー
  if (location.root_.empty()) {
    throw ParserException(ERR_MSG, "root is not set");
  }
  // is_cgi が true の場合、cgi_path が無いとエラー
  if (location.is_cgi_ && location.cgi_path_.empty()) {
    throw ParserException(ERR_MSG, "CGI path is not set");
  }
}

void ConfigParser::AssertMatch(match_type &dest_match,
                               const std::string &match_str) {
  if (match_str == "prefix") {
    dest_match = PREFIX;
  } else if (match_str == "suffix") {
    dest_match = SUFFIX;
  } else {
    throw ParserException(ERR_MSG, (match_str + " is Invalid match").c_str());
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
    throw ParserException(ERR_MSG, (method_str + " is Invalid method").c_str());
  }
  if (dest_method.find(src_method) != dest_method.end()) {
    throw ParserException(ERR_MSG,
                          (method_str + " is Duplicated method").c_str());
  }
  dest_method.insert(src_method);
}

void ConfigParser::AssertAllowMethods(std::set<method_type> &dest_method) {
  if (dest_method.empty()) {
    throw ParserException(ERR_MSG, "Empty method");
  }
}

void ConfigParser::AssertClientMaxBodySize(uint64_t &dest_size,
                                           const std::string &size_str) {
  char unit = 'B';
  std::size_t len = size_str.length();

  if (len == 0) {
    throw ParserException(ERR_MSG, "Empty client_max_body_size");
  }
  if (size_str[len - 1] == 'B' || size_str[len - 1] == 'K' ||
      size_str[len - 1] == 'M' || size_str[len - 1] == 'G') {
    unit = size_str[len - 1];
    len--;
  }
  if (len == 0) {
    throw ParserException(
        ERR_MSG, (size_str + " is Invalid client_max_body_size").c_str());
  }
  if (!ws_strtoi<uint64_t>(&dest_size, size_str.substr(0, len))) {
    throw ParserException(
        ERR_MSG, (size_str + " is Invalid client_max_body_size").c_str());
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
    char unit_str[4] = {'\'', unit, '\'', '\0'};
    throw ParserException(ERR_MSG, (std::string(unit_str) +
                                    " is Invalid client_max_body_size unit")
                                       .c_str());
  }
}

bool AssertPath(const std::string &path) {
  // Linux ファイルシステムの制約に従う
  for (size_t i = 0; i < path.length(); i++) {
    char c = path[i];
    if (!(std::isalnum(c) || c == '.' || c == '_' || c == '-' || c == '~' ||
          c == '+' || c == '%' || c == '@' || c == '#' || c == '$' ||
          c == '&' || c == ',' || c == ';' || c == '=' || c == ':' ||
          c == '|' || c == '^' || c == '!' || c == '*' || c == '`' ||
          c == '(' || c == ')' || c == '{' || c == '}' || c == '<' ||
          c == '>' || c == '?' || c == '[' || c == ']' || c == '\'' ||
          c == '"' || c == '\\' || c == '/')) {
      return false;
    }
  }
  return true;
}

void ConfigParser::AssertRoot(const std::string &root) {
  // root が空文字列、または '/' で始まっていない場合はエラー
  if (root.empty() || root[0] != '/') {
    throw ParserException(
        ERR_MSG, (root + " is Invalid root path. must begin with '/'").c_str());
  }

  // Linux ファイルシステムの制約に従う
  if (!AssertPath(root)) {
    throw ParserException(
        ERR_MSG,
        (root + " is Invalid root path. use Invalid character.").c_str());
  }
}

void ConfigParser::AssertIndex(const std::string &index) {
  // indexディレクティブは、rootディレクティブが設定されている場合のみ有効
  // rootディレクティブを起点にした相対パスである必要がある

  // Linux ファイルシステムの制約に従う
  if (!AssertPath(index)) {
    throw ParserException(
        ERR_MSG,
        (index + " is Invalid index path. use Invalid character.").c_str());
  }
}

void ConfigParser::AssertBool(bool &dest_bool, const std::string &bool_str) {
  if (bool_str == "on") {
    dest_bool = true;
  } else if (bool_str == "off") {
    dest_bool = false;
  } else {
    throw ParserException(ERR_MSG,
                          (bool_str + " is Invalid [on | off]").c_str());
  }
}

void ConfigParser::AssertCgiPath(const std::string &cgi_path) {
  // rootディレクティブを起点にした相対パスである必要がある

  // Linux ファイルシステムの制約に従う
  if (!AssertPath(cgi_path)) {
    throw ParserException(
        ERR_MSG,
        (cgi_path + " is Invalid cgi path. use Invalid character.").c_str());
  }
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
    throw ParserException(ERR_MSG, "Empty error code");
  }
  if (error_page_str.empty()) {
    throw ParserException(ERR_MSG, "Empty error page");
  }
  if (!AssertPath(error_page_str)) {
    throw ParserException(
        ERR_MSG,
        (error_page_str + " is Invalid error page path. use Invalid character.")
            .c_str());
  }
  for (std::vector<int>::const_iterator it = error_codes.begin();
       it != error_codes.end(); it++) {
    if (valid_error_status_codes.count(*it) == 0) {
      throw ParserException("Confg Error: Invalid error code: %d", *it);
    }
    dest_error_pages[*it] = error_page_str;
  }
}

void ConfigParser::AssertReturn(struct Return &return_directive) {
  static const int valid_codes = 57;
  static const int init_list[valid_codes] = {
      200, 201, 202, 203, 204, 205, 206, 207, 208, 226, 300, 301, 302, 303, 304,
      305, 307, 308, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411,
      412, 413, 414, 415, 416, 417, 418, 421, 422, 423, 424, 426, 428, 429, 431,
      451, 500, 501, 502, 503, 504, 505, 506, 507, 508, 510, 511};
  static const std::set<int> valid_status_codes(init_list,
                                                init_list + valid_codes);

  if (valid_status_codes.count(return_directive.status_code_) == 0) {
    throw ParserException("Confg Error: Invalid return code: %d",
                          return_directive.status_code_);
  }
}

// utils
void ConfigParser::Expect(const char c) {
  if (IsEof()) {
    throw ParserException("Unexpected EOF");
  }
  if (*it_ != c) {
    // エラー箇所の行数と列数を計算する
    int row = 1;
    int column = 1;
    std::string line; // エラー箇所の行の内容を表示するために使う
    this->GetErrorPoint(row, column, line);

    throw ParserException("Config Error: Expected char is [%c], but [%c].\nrow "
                          "%d: column %d\n%s\n%*c%c",
                          c, *it_, row, column, line.c_str(), column, ' ', '^');
  }
  it_++;
}

void ConfigParser::SkipSpaces() {
  while (!this->IsEof() && isspace(*it_)) {
    it_++;
  }
}

void ConfigParser::GetErrorPoint(int &row, int &column, std::string &line) {
  // エラー箇所の行数と列数を計算する
  row = 1;
  column = 1;
  std::string::const_iterator line_begin = it_;
  while (line_begin != file_content_.begin() && *line_begin != '\n') {
    line_begin--;
  }
  if (*line_begin == '\n') {
    line_begin++;
    column = it_ - line_begin;
  }
  std::string::const_iterator line_end = it_;
  while (line_end != file_content_.end() && *line_end != '\n') {
    line_end++;
  }
  line = std::string(line_begin, line_end);
}

std::string ConfigParser::GetWord() {
  std::string word;
  while (!this->IsEof() && !this->IsDelim() && !isspace(*it_)) {
    word += *it_++;
  }
  if (word.empty()) {
    int row = 1;
    int column = 1;
    std::string line; // エラー箇所の行の内容を表示するために使う
    this->GetErrorPoint(row, column, line);
    throw ParserException(
        "Config Error: Empty Word.\nrow %d: column %d\n%s\n%*c%c", row, column,
        line.c_str(), column, ' ', '^');
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

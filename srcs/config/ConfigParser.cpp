#include "ConfigParser.hpp"
#include "define.hpp"
#include "utils.hpp"
#include <fstream>
#include <iostream>
#include <netdb.h> // for getaddrinfo()
#include <sstream>
#include <stdarg.h>

// 行、列、行の内容、エラーの理由を出力する
#define ERR_MSG_ROW_COL_LINE "Config Error: row %d, col %d\n%s <--- %s\n"

// エラー理由を出力する
#define ERR_MSG "Config Error: %s\n"

ConfigParser::ConfigParser(const char *filepath)
    : file_content_(LoadFile(filepath)), it_(file_content_.begin()) {
  DEBUG_PRINT("ConfigParser() filepath: %s\n", filepath);
  DEBUG_PRINT("ConfigParser() file_content_: %s\n", file_content_.c_str());
}

ConfigParser::~ConfigParser() {}

// コンストラクタ内で使用
std::string GetFileExt(const char *filepath) {
  std::string path(filepath);
  std::string::size_type idx = path.rfind('.');
  if (idx == std::string::npos) {
    return "";
  }
  return path.substr(idx + 1); // substr()の第1引数は pos <= size() を許容する
}

// コンストラクタ内で使用
std::string ConfigParser::LoadFile(const char *filepath) {
  // filepathの拡張子を確認
  std::string ext = GetFileExt(filepath);
  if (ext != "conf") {
    throw ParserException("Config Error: invalid file extension: %s",
                          ext.c_str());
  }

  // ファイルを読み込み、stringに変換
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
  while (!IsEof()) {
    SkipSpaces();
    if (GetWord() != "server") {
      ThrowParseError("Expected 'server'", true);
    }
    ParseServer(config);
    SkipSpaces();
  }
  AssertConfig(config);
}

void ConfigParser::ParseServer(Config &config) {
  Vserver server;

  SkipSpaces(true);
  Expect('{');
  while (!IsEof() && *it_ != '}') {
    SkipSpaces();
    std::string token = GetWord();
    if (token == "listen") {
      ParseListen(server);
    } else if (token == "server_name") {
      ParseServerName(server);
    } else if (token == "timeout") {
      ParseTimeOut(server);
    } else if (token == "location") {
      ParseLocation(server);
    } else {
      ThrowParseError("Unexpected token", true);
    }
    SkipSpaces();
  }
  Expect('}');
  AssertServer(server);
  config.AddServer(server);
}

void ConfigParser::ParseListen(Vserver &server) {
  SkipSpaces(true);
  std::string listen_str = GetWord();
  SkipSpaces(true);
  Expect(';');
  AssertListen(server.listen_, listen_str);
}

void ConfigParser::ParseServerName(Vserver &server) {
  std::vector<std::string> new_server_names;

  while (!IsEof() && *it_ != ';') {
    SkipSpaces(true);
    std::string server_name = GetWord();
    SkipSpaces(true);
    AssertServerName(server_name);
    new_server_names.push_back(server_name);
  }
  Expect(';');
  server.server_names_ = new_server_names;
}

void ConfigParser::ParseTimeOut(Vserver &server) {
  SkipSpaces(true);
  std::string timeout_str = GetWord();
  SkipSpaces(true);
  Expect(';');
  AssertTimeOut(server.timeout_, timeout_str);
}

void ConfigParser::ParseLocation(Vserver &server) {

  Location location;

  SkipSpaces(true);
  location.path_ = GetWord();
  SkipSpaces(true);
  Expect('{');
  while (!IsEof() && *it_ != '}') {
    SkipSpaces();
    std::string token = GetWord();
    if (token == "allow_method") {
      ParseAllowMethod(location);
    } else if (token == "client_max_body_size") {
      ParseClientMaxBodySize(location);
    } else if (token == "root") {
      ParseRoot(location);
    } else if (token == "index") {
      ParseIndex(location);
    } else if (token == "cgi_extension") {
      ParseCgiExtension(location);
    } else if (token == "error_page") {
      ParseErrorPages(location);
    } else if (token == "autoindex") {
      ParseAutoIndex(location);
    } else if (token == "return") {
      ParseReturn(location);
    } else {
      ThrowParseError("Unexpected token", true);
    }
    SkipSpaces();
  }
  Expect('}');
  AssertLocation(location);
  server.locations_.push_back(location);
}

void ConfigParser::ParseAllowMethod(Location &location) {
  location.allow_methods_.clear();
  while (!IsEof() && *it_ != ';') {
    SkipSpaces(true);
    std::string method_str = GetWord();
    SkipSpaces(true);
    AssertAllowMethod(location.allow_methods_, method_str);
  }
  Expect(';');
  AssertAllowMethods(location.allow_methods_);
}

void ConfigParser::ParseClientMaxBodySize(Location &location) {
  SkipSpaces(true);
  std::string size_str = GetWord();
  SkipSpaces(true);
  Expect(';');
  AssertClientMaxBodySize(location.client_max_body_size_, size_str);
}

void ConfigParser::ParseRoot(Location &location) {
  SkipSpaces(true);
  location.root_ = GetWord();
  SkipSpaces(true);
  Expect(';');
  AssertRoot(location.root_);
}

void ConfigParser::ParseIndex(Location &location) {
  SkipSpaces(true);
  location.index_ = GetWord();
  SkipSpaces(true);
  AssertIndex(location.index_);
  Expect(';');
}

void ConfigParser::ParseCgiExtension(Location &location) {
  location.cgi_extensions_.clear(); // 複数回ある場合は上書き
  SkipSpaces(true);
  while (!IsEof() && *it_ != ';') {
    std::string extension_str = GetWord();
    SkipSpaces(true);
    AssertCgiExtension(location.cgi_extensions_, extension_str);
  }
  Expect(';');
  AssertCgiExtensions(location.cgi_extensions_);
}

void ConfigParser::ParseErrorPages(Location &location) {
  while (!IsEof() && *it_ != ';') {
    std::vector<int> error_codes;
    std::string error_page_str;
    while (true) {
      int error_code;
      SkipSpaces(true);
      error_page_str = GetWord();
      SkipSpaces(true);
      if (!ws_strtoi(&error_code, error_page_str)) {
        break;
      }
      error_codes.push_back(error_code);
    }
    AssertErrorPages(location.error_pages_, error_codes, error_page_str);
  }
  Expect(';');
}

void ConfigParser::ParseAutoIndex(Location &location) {
  SkipSpaces(true);
  std::string autoindex_str = GetWord();
  SkipSpaces(true);
  Expect(';');
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

  SkipSpaces(true);
  std::string tmp_str = GetWord();

  // １つ目のワードが、status_codeかURLorTextかを判定
  if (ws_strtoi(&location.return_.status_code_, tmp_str)) {
    // status codeが設定されたので、URLかtextかを判定
    // textはシングルクォーテーションで囲まれている
    // URLはシングルクォーテーションで囲まれていない
    SkipSpaces(true);
    if (*it_ == ';') { // status codeのみ指定された場合
      location.return_.return_type_ = RETURN_ONLY_STATUS_CODE;
    } else {
      tmp_str = GetWord();
      if (tmp_str.size() >= 2 && tmp_str[0] == '\'' &&
          tmp_str[tmp_str.size() - 1] == '\'') {
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
  SkipSpaces(true);
  Expect(';');
  AssertReturn(location.return_);
}

// validator
void ConfigParser::AssertConfig(const Config &config) {
  if (config.GetServerVec().empty()) {
    throw ParserException(ERR_MSG, "server brock is not set");
  }
}

void ConfigParser::AssertServer(Vserver &server) {
  // server_name ディレクティブが無い場合も許容する
  // if (server.server_names_.empty()) {
  //   throw ParserException(ERR_MSG, "server name is not set");
  // }

  // location が設定されているかを確認
  if (server.locations_.empty()) {
    throw ParserException(ERR_MSG, "location is not set");
  }
  // location path に / があることを保証する
  for (std::vector<Location>::size_type i = 0; i < server.locations_.size();
       i++) {
    if (server.locations_[i].path_ == "/") { // / があればOKなのでreturn
      // /のlocationを先頭に持ってくる
      // c++98ではswapが使えないので、 tmpを使ってswapする
      Location tmp = server.locations_[0];
      server.locations_[0] = server.locations_[i];
      server.locations_[i] = tmp;

      return;
    }
  }
  // location path に重複が無いことを保証する
  std::set<std::string> location_path_set;
  for (std::vector<Location>::size_type i = 0; i < server.locations_.size();
       i++) {
    if (location_path_set.find(server.locations_[i].path_) !=
        location_path_set.end()) {
      throw ParserException(ERR_MSG, "location path is duplicated");
    }
    location_path_set.insert(server.locations_[i].path_);
  }
  // for文を抜けてきた場合、/ がないのでエラー
  throw ParserException(ERR_MSG, "location / is not set");
}

void ConfigParser::AssertListen(struct sockaddr_in &dest_listen,
                                const std::string &listen_str) {

  std::string host_str;
  std::string port_str;

  std::size_t colon_pos = listen_str.find(':');
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
  ws_memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_INET;
  if ((err = getaddrinfo(host_str.c_str(), port_str.c_str(), &hints, &res)) !=
      0) {
    throw ParserException(ERR_MSG, gai_strerror(err));
  }
  if (res->ai_family != AF_INET) {
    throw ParserException(ERR_MSG, "not AF_INET");
  }
  ws_memcpy(&dest_listen, res->ai_addr, sizeof(struct sockaddr_in));
  freeaddrinfo(res);
  return;
}

void ConfigParser::AssertServerName(const std::string &server_name) {
  if (server_name.empty()) {
    throw ParserException(ERR_MSG, "Empty server name");
  }
  uint32_t ip_addr; // dummy
  if (ws_inet_addr(ip_addr, server_name.c_str())) {
    // IPアドレスに変換できた場合はIPアドレスとして扱う
  } else {
    // IPアドレスに変換できなかった場合はドメイン名としてValidationする
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
}

void ConfigParser::AssertTimeOut(int &dest_timeout,
                                 const std::string &timeout_str) {
  if (!ws_strtoi<int>(&dest_timeout, timeout_str)) {
    throw ParserException(ERR_MSG,
                          (timeout_str + " is Invalid timeout").c_str());
  }
}

void ConfigParser::AssertLocation(const Location &location) {
  // location path 有効かチェック
  if (!IsValidPath(location.path_)) {
    throw ParserException(ERR_MSG, "location path is invalid");
  }
  // root ディレクティブが無いとエラー
  if (location.root_.empty()) {
    throw ParserException(ERR_MSG, "root is not set");
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
  try {
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
  } catch (const std::overflow_error &e) {
    throw ParserException(
        ERR_MSG, (std::string(size_str) +
                  " is Invalid 'uint64_t' overflow client_max_body_size")
                     .c_str());
  }
}

void ConfigParser::AssertRoot(const std::string &root) {
  // root が空文字列、または '/' で始まっていない場合はエラー
  if (root.empty() || root[0] != '/') {
    throw ParserException(
        ERR_MSG, (root + " is Invalid root path. must begin with '/'").c_str());
  }

  // Linux ファイルシステムの制約に従う
  if (!IsValidPath(root)) {
    throw ParserException(
        ERR_MSG,
        (root + " is Invalid root path. use Invalid character.").c_str());
  }
}

void ConfigParser::AssertIndex(const std::string &index) {
  // rootディレクティブを起点にした相対パスである必要がある

  // Linux ファイルシステムの制約に従う
  if (!IsValidPath(index)) {
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

void ConfigParser::AssertCgiExtension(std::vector<std::string> &cgi_extensions_,
                                      const std::string &cgi_extension) {
  if (cgi_extension[0] != '.') {
    throw ParserException(
        ERR_MSG, (cgi_extension + " is Invalid cgi extension").c_str());
  }

  // 拡張子で使えない文字が含まれていないかチェック '/' は使えない
  for (std::size_t i = 0; i < cgi_extension.size(); i++) {
    if (cgi_extension[i] == '/') {
      throw ParserException(
          ERR_MSG,
          (cgi_extension + " is Invalid cgi extension. use Invalid character.")
              .c_str());
    }
  }
  // '/'が含まれていないことを保証したうえで、Linux ファイルシステムの制約に従う
  if (IsValidPath(cgi_extension)) {
    cgi_extensions_.push_back(cgi_extension);
  } else {
    throw ParserException(
        ERR_MSG,
        (cgi_extension + " is Invalid cgi extension. use Invalid character.")
            .c_str());
  }
}

void ConfigParser::AssertCgiExtensions(
    std::vector<std::string> &cgi_extensions_) {
  if (cgi_extensions_.empty()) {
    throw ParserException(ERR_MSG, "Empty cgi extension");
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
  if (!IsValidPath(error_page_str)) {
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
  if (return_directive.return_type_ == RETURN_URL &&
      !IsValidUrl(return_directive.return_url_)) {
    throw ParserException(ERR_MSG,
                          (return_directive.return_url_ +
                           " is Invalid return url. use Invalid character.")
                              .c_str());
  }
}

// is

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

bool ConfigParser::IsValidUrl(const std::string &url) {
  if (url.empty()) {
    return false;
  }

  // Relative URL
  if (url[0] == '/') {
    if (IsValidPath(url)) {
      return true;
    } else {
      return false;
    }
  }

  // Absolute URL should start with http:// or https://
  if (url.substr(0, 7) != "http://" && url.substr(0, 8) != "https://") {
    return false;
  }
  // URL should have a valid domain name after http:// or https://
  // URLからドメイン名だけを抽出
  std::size_t start = url.find_first_of(':') + 3;
  std::size_t end = url.find_first_of('/', start);
  if (end == std::string::npos) {
    end = url.length();
  }
  std::string domain = url.substr(start, end - start);
  AssertServerName(domain);
  return true;
}

bool ConfigParser::IsValidPath(const std::string &path) {
  // Linux ファイルシステムの制約に従う
  for (std::size_t i = 0; i < path.length(); i++) {
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
    GetErrorPoint(row, column, line);

    throw ParserException("Config Error: Expected char is [%c], but [%c].\nrow "
                          "%d: column %d\n%s\n%*c%c",
                          c, *it_, row, column, line.c_str(), column, ' ', '^');
  }
  it_++;
}

// デフォルトでは、空白文字と改行文字をスキップする
// 引数をtrueにすると、改行文字をスキップしない
// ディレクティブとそのパラメータの間、ディレクティブの終了のセミコロンの間には改行が許容されないので、
// そのような箇所では改行をスキップしないようにtrueを渡して呼び出す
void ConfigParser::SkipSpaces(bool skip_newline) {
  if (skip_newline) {
    while (!IsEof() && ws_isspace(*it_)) {
      it_++;
    }
  } else {
    while (!IsEof() && *it_ != ' ' && *it_ != '\t') {
      it_++;
    }
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
  // エラー箇所の行数を計算する
  std::stringstream ss(file_content_);
  std::string tmp;
  while (std::getline(ss, tmp)) {
    if (tmp == line) {
      break;
    }
    row++;
  }
}

std::string ConfigParser::GetWord() {
  std::string word;
  while (!IsEof() && !IsDelim() && !isspace(*it_)) {
    word += *it_++;
  }
  if (word.empty()) {
    int row = 1;
    int column = 1;
    std::string line; // エラー箇所の行の内容を表示するために使う
    GetErrorPoint(row, column, line);
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

// parse error時に、例外をスローする.
// 第二引数がtrueの場合は、行、列、行の内容も出力する
void ConfigParser::ThrowParseError(const char *msg, bool add_err_point_flag) {
  if (add_err_point_flag) {
    int row;
    int col;
    std::string line; // エラー箇所の行の内容を表示するために使う
    GetErrorPoint(row, col, line);
    throw ParserException(ERR_MSG_ROW_COL_LINE, row, col, line.c_str(), msg);
  } else {
    throw ParserException(ERR_MSG, msg);
  }
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

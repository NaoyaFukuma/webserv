#ifndef CONFIG_PARSER_HPP_
#define CONFIG_PARSER_HPP_

#include "Config.hpp"
#include <string>
#include <vector>

class ConfigParser {
public:
  ConfigParser(const char *filepath);
  ~ConfigParser();

  void Parse(Config &config);

  class ParserException : public std::exception {
  private:
    static const int MAX_ERROR_LEN = 1024;

  public:
    ParserException(const char *errfmt = "Parser error.", ...);
    const char *what() const throw();

  private:
    char errmsg_[MAX_ERROR_LEN];
  };

private:
  const std::string file_content_;
  std::string::const_iterator it_;

  // ピリオドを含むドメイン全体の長さ
  static const int kMaxDomainLength = 253;
  // ドメインの各ラベル(ピリオド区切りの文字列)の最大長
  static const int kMaxDomainLabelLength = 63;
  // ポート番号の最大値
  static const int kMaxPortNumber = 65535;

  std::string LoadFile(const char *filepath);

  // parser
  void ParseServer(Config &config);
  void ParseListen(Vserver &server);
  void ParseServerName(Vserver &server);
  void ParseTimeOut(Vserver &server);
  void ParseLocation(Vserver &server);
  void ParseAllowMethod(Location &location);
  void ParseClientMaxBodySize(Location &location);
  void ParseRoot(Location &location);
  void ParseIndex(Location &location);
  void ParseCgiExtension(Location &location);
  void ParseErrorPages(Location &location);
  void ParseAutoIndex(Location &location);
  void ParseReturn(Location &location);

  // validator
  void AssertConfig(const Config &config);
  void AssertServer(Vserver &server);
  void AssertListen(struct sockaddr_in &dest_listen,
                    const std::string &listen_str);
  void AssertServerName(const std::string &server_name);
  void AssertTimeOut(int &timeout, const std::string &timeout_str);
  void AssertLocation(const Location &location);
  void AssertAllowMethod(std::set<method_type> &dest_method,
                         const std::string &method_str);
  void AssertAllowMethods(std::set<method_type> &dest_method);
  void AssertClientMaxBodySize(uint64_t &dest_size,
                               const std::string &size_str);
  void AssertRoot(const std::string &root);
  void AssertIndex(const std::string &index);
  void AssertCgiExtension(std::vector<std::string> &cgi_extensions_,
                          const std::string &cgi_extension);
  void AssertCgiExtensions(std::vector<std::string> &cgi_extensions_);
  void AssertErrorPages(std::map<int, std::string> &dest_error_pages,
                        const std::vector<int> error_codes,
                        const std::string &error_page_str);
  void AssertBool(bool &dest_bool, const std::string &bool_str);
  void AssertReturn(struct Return &return_directive);

  // is
  bool IsValidIp(const std::string &ip_str);
  bool IsValidLabel(const std::string &server_name,
                    std::string::const_iterator &it);
  bool IsValidUrl(const std::string &url);
  bool IsValidPath(const std::string &path);

  // utils
  void Expect(const char c);
  void SkipSpaces(bool skip_newline = true);
  std::string GetWord();
  void GetErrorPoint(int &row, int &column, std::string &line);
  void ThrowParseError(const char *msg, bool err_msg_only = true);

  bool IsEof();
  bool IsDelim();

  // 不使用だが、コンパイラが自動生成し、予期せず使用するのを防ぐために記述
  ConfigParser();
  ConfigParser(const ConfigParser &other);
  ConfigParser &operator=(const ConfigParser &other);
};

#endif // CONFIG_PARSER_HPP_

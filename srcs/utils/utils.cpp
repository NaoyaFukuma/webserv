#include "utils.hpp"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <sys/stat.h>
#include <vector>

// partial_pathがcgi_extensionで終わり、かつregular
// fileであれば、cgiとして実行する
bool ws_exist_cgi_file(const std::string &path, const std::string &extension) {
  return ((extension == "." || end_with(path, extension)) &&
          get_filetype(path) == FILE_REGULAR);
}

// ファイル名からMIMEタイプを取得する関数
std::string ws_get_mime_type(const std::string &filename) {
  static std::map<std::string, std::string> extensionToMime;
  if (extensionToMime.empty()) {
    extensionToMime[".html"] = "text/html";
    extensionToMime[".css"] = "text/css";
    extensionToMime[".js"] = "application/javascript";
    extensionToMime[".jpg"] = "image/jpeg";
    extensionToMime[".jpeg"] = "image/jpeg";
    extensionToMime[".png"] = "image/png";
    extensionToMime[".gif"] = "image/gif";
    // 他の拡張子とMIMEタイプの対応もここに追加
  }

  std::string::size_type dotPos = filename.rfind('.');
  if (dotPos != std::string::npos) {
    std::string extension = filename.substr(dotPos);
    if (extensionToMime.count(extension) > 0) {
      return extensionToMime[extension];
    } else {
      // 拡張子が対応表にない場合、適当なデフォルト値を返すか、エラーを返すなど
      return "application/octet-stream"; // 一例として、バイナリデータのMIMEタイプを返す
    }
  } else {
    // ファイル名に拡張子がない場合、適当なデフォルト値を返すか、エラーを返すなど
    return "application/octet-stream"; // 一例として、バイナリデータのMIMEタイプを返す
  }
}

ssize_t ws_split(std::vector<std::string> &dst, const std::string &src,
                 const char delim) {
  std::istringstream iss(src);
  if (!iss) {
    return -1;
  }
  std::vector<std::string> tokens;
  std::string token;

  while (std::getline(iss, token, delim)) {
    if (iss.fail() && !iss.eof()) {
      return -1;
    }
    tokens.push_back(token);
  }
  dst = tokens;

  return tokens.size();
}

bool ws_inet_addr(uint32_t &dst, std::string ip) {
  uint32_t res = 0;
  uint32_t tmp = 0;
  uint32_t cnt = 0;

  while (ip.find(".") != std::string::npos) {
    cnt++;
    if (cnt > 3 ||
        ws_strtoi<uint32_t>(&tmp, ip.substr(0, ip.find("."))) == false ||
        tmp > 255) {
      return false;
    }
    res = (res << 8) + tmp;
    ip = ip.substr(ip.find(".") + 1);
  }
  if (ws_strtoi<uint32_t>(&tmp, ip) == false || tmp > 255) {
    return false;
  }
  res = (res << 8) + tmp;
  dst = res;
  return true;
}

void *ws_memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s;
  while (n--) {
    *p++ = (unsigned char)c;
  }
  return s;
}

void *ws_memcpy(void *dest, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;
  while (n--) {
    *d++ = *s++;
  }
  return dest;
}

bool ws_isspace(const char c) {
  if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
      c == '\f') {
    return true;
  }
  return false;
}

bool end_with(const std::string &str, const std::string &query) {
  if (str.size() >= query.size() &&
      str.substr(str.size() - query.size()) == query) {
    return true;
  }
  return false;
}

FileType get_filetype(const std::string &path) {
  struct stat s;

  if (stat(path.c_str(), &s) == 0) {
    if (s.st_mode & S_IFDIR) {
      // it's a directory
      return FILE_DIRECTORY;
    } else if (s.st_mode & S_IFREG) {
      // it's a file
      return FILE_REGULAR;
    } else {
      // something else
      return FILE_OTHERS;
    }
  } else {
    // error or unknown
    return FILE_UNKNOWN;
  }
}

std::string get_date() { return time2str(std::time(NULL)); }

std::time_t str2time(std::string time_str) {
  struct std::tm tm = {};
  char *result = strptime(time_str.c_str(), "%a, %d %b %Y %H:%M:%S", &tm);

  if (result == NULL) {
    return -1;
  }

  std::time_t time = std::mktime(&tm);
  return time;
}

std::string time2str(std::time_t time) {
  char buffer[128];
  std::tm *tm = std::gmtime(&time);

  std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %T GMT", tm);
  std::string str(buffer);

  return str;
}

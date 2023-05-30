#include "Response.hpp"
#include "CgiSocket.hpp"
#include "Epoll.hpp"
#include "Socket.hpp"
#include "utils.hpp"
#include <dirent.h>
#include <fstream>
#include <sys/epoll.h>
#include <vector>

Response::Response() { process_status_ = PROCESSING; }

Response::~Response() {}

Response::Response(const Response &src) { *this = src; }

Response &Response::operator=(const Response &src) {
  if (this != &src) {
    process_status_ = src.process_status_;
    status_code_ = src.status_code_;
    status_message_ = src.status_message_;
    header_ = src.header_;
    body_ = src.body_;
  }
  return *this;
}

std::string Response::GetString() {
  std::string ret;
  ret += Http::VersionToString(version_) + " " + std::to_string(status_code_) +
         " " + status_message_ + "\r\n";
  // Todo: vector形式のvalueの扱い方について調べる
  ret += "Date: " + get_date() + "\r\n";
  for (Header::iterator ith = header_.begin(); ith != header_.end(); ++ith) {
    for (std::vector<std::string>::iterator itv = ith->second.begin();
         itv != ith->second.end(); ++itv) {
      ret += ith->first + ": " + *itv + "\r\n";
    }
  }
  ret += "\r\n";
  ret += body_;
  return ret;
}

ProcessStatus Response::GetProcessStatus() const { return process_status_; }

std::vector<std::string> Response::GetHeader(const std::string &key) {
  return header_[key];
}

bool Response::HasHeader(const std::string &key) const {
  return header_.find(key) != header_.end();
}

void Response::SetResponseStatus(Http::HttpStatus status) {
  status_code_ = status.status_code;
  status_message_ = status.message;
}

void Response::SetVersion(Http::Version version) { version_ = version; }

void Response::SetHeader(std::string key, std::string value) {
  if (header_.find(key) != header_.end()) {
    header_[key].push_back(value);
  } else {
    header_[key] = std::vector<std::string>{value};
  }
}

void Response::SetBody(std::string body) {
  SetHeader("Content-Length", std::to_string(body.length()));
  body_ += body;
}

void Response::ProcessRequest(Request &request, ConnSocket *socket,
                              Epoll *epoll) {
  // request.ResolvePath(config);
  if (request.GetContext().is_cgi) {
    // CGI
    CgiSocket *cgi_socket = new CgiSocket(socket, request);
    ASocket *sock = cgi_socket->CreatCgiProcess();
    if (sock == NULL) {
      delete cgi_socket;
      // エラー処理 クライアントへ500 Internal Server Errorを返す
      return;
    }
    uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    epoll->Add(sock, event_mask);
  } else {
    // 静的ファイル
    ProcessStatic(request, socket, epoll);
  }
}

void Response::ProcessStatic(Request &request, ConnSocket *socket,
                             Epoll *epoll) {
  // parseの時点でerrorがあった場合はこの時点で返す
  if (request.GetRequestStatus().status_code != -1) {
    SetResponseStatus(request.GetRequestStatus());
    process_status_ = DONE;
    epoll->Mod(socket->GetFd(), EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
    return;
  }
  // Todo: resolvepathはrequest parserの時点で行う
  request.ResolvePath(socket->GetConfVec());
  const Context &context = request.GetContext();
  if (context.location.return_.return_type_ != RETURN_EMPTY) {
    ProcessReturn(request, socket, epoll);
    return;
  }
  if (context.is_cgi) {
    // CGI
  } else {
    // 静的ファイル
    ProcessStatic(request, socket, epoll);
  }
}

void Response::ProcessReturn(Request &request, ConnSocket *socket,
                             Epoll *epoll) {
  const Return &return_ = request.GetContext().location.return_;
  SetResponseStatus(Http::HttpStatus(return_.status_code_));
  switch (return_.return_type_) {
  case RETURN_ONLY_STATUS_CODE:
    break;
  case RETURN_TEXT:
    SetBody(return_.return_text_);
    break;
  case RETURN_URL:
    SetHeader("Location", return_.return_url_);
    break;
  }
  epoll->Mod(socket->GetFd(), EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
  process_status_ = DONE;
}

void Response::ProcessStatic(Request &request, ConnSocket *socket,
                             Epoll *epoll) {
  RequestMessage message = request.GetRequestMessage();
  if (message.request_line.method == "GET") {
    ProcessGET(request);
  } else if (message.request_line.method == "DELETE") {
    ProcessDELETE(request);
  } else {
    // 405 Method Not Allowed
    SetResponseStatus(Http::HttpStatus(405));
  }
  epoll->Mod(socket->GetFd(), EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
  process_status_ = DONE;
}

void Response::ProcessGET(Request &request) {
  // 指定されたpathがディレクトリだった場合、index, autoindexを解決する
  // indexの解決: location path + index -> 存在しなければautoindexのチェック
  // autoindexの解決:
  Context context = request.GetContext();
  std::string path = context.resource_path.server_path;
  FileType ftype = get_filetype(path);

  if (ftype == FileType::FILE_DIRECTORY) {
    if (!context.location.index_.empty() &&
        get_filetype(path + '/' + context.location.index_) ==
            FileType::FILE_REGULAR) {
      GetFile(request, path + '/' + context.location.index_);
    } else if (context.location.autoindex_) {
      // autoindex
      ProcessAutoindex(request, path);
    } else {
      // 404 Not Found
      SetResponseStatus(Http::HttpStatus(404));
    }
  } else if (ftype == FileType::FILE_REGULAR) {
    GetFile(request, path);
  } else {
    // 404 Not Found
    SetResponseStatus(Http::HttpStatus(404));
  }
}

void Response::ProcessDELETE(Request &request) {
  // 指定されたpathがディレクトリだった場合、index, autoindexを解決する
  // indexの解決: location path + index -> 存在しなければautoindexのチェック
  // autoindexの解決:
  Context context = request.GetContext();
  std::string path = context.resource_path.server_path;
  FileType ftype = get_filetype(path);

  if (ftype == FileType::FILE_DIRECTORY) {
    // directoryの削除は認めない
    SetResponseStatus(Http::HttpStatus(403));
  } else if (ftype == FileType::FILE_REGULAR) {
    DeleteFile(request, path);
  } else {
    // 404 Not Found
    SetResponseStatus(Http::HttpStatus(404));
  }
}

void Response::GetFile(Request &request, const std::string &path) {
  if (IsGetableFile() == false && status_code_ != 206) {
    return;
  }
  StaticFileBody(path);
}

void Response::DeleteFile(request, path) {
  if (IsDeletableFile() == false) {
    return;
  }
  if (unlink(path.c_str()) == -1) {
    SetResponseStatus(Http::HttpStatus(500));
  } else {
    SetResponseStatus(Http::HttpStatus(200));
  }
}

void Response::ProcessAutoindex(Request &request, const std::string &path) {
  DIR *dir = opendir(path.c_str());
  if (dir == NULL) {
    SetResponseStatus(Http::HttpStatus(500));
  } else {
    ResFileList(dir);
    closedir(dir);
  }
}

void Response::ResFileList(DIR *dir) {
  std::stringstream ss;
  ss << "<html><head><title>File List</title></head><body>\r\n";
  ss << "<h1>File List</h1>\r\n";
  ss << "<ul>\r\n";
  struct dirent *dp;
  while ((dp = readdir(dir)) != NULL) {
    // ディレクトリはスキップ
    if (dp->d_type == DT_DIR) {
      continue;
    }
    ss << "<li><a href=\"/upload/" << dp->d_name << "\">" << dp->d_name
       << "</a></li>\r\n";
  }
  ss << "</ul>\r\n";
  ss << "</body></html>\r\n";

  SetResponseStatus(Http::HttpStatus(200));
  SetHeader("Content-Type", "text/html");
  SetBody(ss.str());
}

bool Response::StaticFileBody(const std::string &path) {
  std::ifstream ifs(path, std::ios::binary);
  if (!ifs) {
    // 500 Internal Server Error
    SetResponseStatus(Http::HttpStatus(500));
    return;
  }
  // sizeを取得
  size_t start;
  size_t end;
  if (!ranges_.empty()) {
    start = ranges_[0].first;
    end = ranges_[0].second;
  } else {
    start = 0;
    end = ifs.seekg(0, std::ios::end).tellg();
  }
  size_t body_size = end - start;
  // sizeがkMaxBodyLengthを超えていたら、500 Internal Server Error
  if (body_size > kMaxBodyLength || end > ifs.seekg(0, std::ios::end).tellg()) {
    // 500 Internal Server Error
    SetResponseStatus(Http::HttpStatus(500));
    return;
  }
  std::string body;
  body.resize(body_size);
  ifs.seekg(0, std::ios::beg).read(&body[start], body_size);
  SetBody(body);

  // content-typeを設定
<<<<<<< HEAD
  SetHeader("Content-Type",
            std::vector<std::string>(1, ws_get_mime_type(path.c_str())));
||||||| parent of 15645d4 (if~~のheaderチェックを追加)
  // SetHeader("Content-Type",
  //           std::vector<std::string>(1, ws_get_mime_type(path.c_str())));

  // content-lengthを設定
  std::stringstream ss;
  ss << body_size;
  SetHeader("Content-Length", ss.str());

  // // Check If headers.
  // iIfMod = IfModSince(hInfo, sBuf.st_mtime);
  // iIfUnmod = IfUnmodSince(hInfo, sBuf.st_mtime);
  // iIfMatch = IfMatch(hInfo, sBuf.st_mtime);
  // iIfNone = IfNone(hInfo, sBuf.st_mtime);
  // iIfRange = IfRange(hInfo, sBuf.st_mtime);
  // iRangeErr = hInfo->FindRanges(sBuf.st_size);

  // // Check to make sure any If headers are FALSE.
  // // Either not-modified or no etags matched.
  // if ((iIfMod == FALSE) || (iIfNone == FALSE)) {
  //   sClient->Send("HTTP/1.1 304 Not Modified\r\n");
  //   iRsp = 304;
  // }
  // // No matching etags or it's been modified.
  // else if ((iIfMatch == FALSE) || (iIfUnmod == FALSE)) {
  //   sClient->Send("HTTP/1.1 412 Precondition Failed\r\n");
  //   iRsp = 412;
  // }
  // // Resource matched so send just the bytes requested.
  // else if ((iIfRange == TRUE) && (iRangeErr == 0)) {
  //   sClient->Send("HTTP/1.1 206 Partial Content\r\n");
  //   iRsp = 206;
  // }
  // // Resource didn't match, so send the entire entity.
  // else {
  //   sClient->Send("HTTP/1.1 200 OK\r\n");
  //   iRsp = 200;
  // }
=======
  // SetHeader("Content-Type",
  //           std::vector<std::string>(1, ws_get_mime_type(path.c_str())));

  // content-lengthを設定
  std::stringstream ss;
  ss << body_size;
  SetHeader("Content-Length", ss.str());
}

bool Response::IsValidFile(Request &request, const std::string &path) {
  // Check If headers.
  bool iIfMod = IfModSince(request, path);
  bool iIfUnmod = IfUnmodSince(request, path);
  bool iIfMatch = IfMatch(request, path);
  bool iIfNone = IfNone(request, path);
  bool iIfRange = IfRange(request, path);
  std::vector<std::pair<size_t, size_t>> ranges;
  bool iFindRange = FindRanges(request, path, ranges);

  // Check to make sure any If headers are FALSE.
  // Either not-modified or no etags matched.
  if ((iIfMod == false) || (iIfNone == false)) {
    SetResponseStatus(304);
    return false;
  }
  // No matching etags or it's been modified.
  else if ((iIfMatch == false) || (iIfUnmod == false)) {
    SetResponseStatus(412);
    return false;
  }
  // Resource matched so send just the bytes requested.
  else if ((iIfRange == true) && (iFindRange == true)) {
    SetResponseStatus(206);
    // Todo: set partial
    // body(バイトレンジを指定している場合、bodyに指定された部分のみを設定する)
    return false;
  }
  // Resource didn't match, so send the entire entity.
  else {
    return true;
  }
}

bool Response::IfModSince(Request &request, const std::string &path) {
  // If-Modified-Since
  if (request.HasHeader("If-Modified-Since") == false ||
      request.GetHeader("If-Modified-Since").empty()) {
    return true;
  }
  std::string if_mod_since_str = request.GetHeader("If-Modified-Since")[0];
  std::time_t if_mod_since = str2time(if_mod_since_str);
  if (if_mod_since == -1) {
    return true;
  }
  std::time_t last_mod = GetLastModified(path);
  return last_mod > if_mod_since;
}

bool Response::IfUnmodSince(Request &request, const std::string &path) {
  if (request.HasHeader("If-Unmodified-Since") == false ||
      request.GetHeader("If-Unmodified-Since").empty()) {
    return true;
  }
  std::string if_unmod_since_str = request.GetHeader("If-Unmodified-Since")[0];
  std::time_t if_unmod_since = str2time(if_unmod_since_str);
  if (if_unmod_since == -1) {
    return true;
  }
  std::time_t last_mod = GetLastModified(path);
  return last_mod < if_unmod_since;
}

bool Response::IfMatch(Request &request, const std::string &path) {
  if (request.HasHeader("If-Match") == false ||
      request.GetHeader("If-Match").empty()) {
    return true;
  }
  std::vector<std::string> if_match_vec = request.GetHeader("If-Match");
  std::string etag = GetEtag(path);
  for (std::string::iterator if_match = if_match_vec.begin();
       if_match != if_match_vec.end(); if_match++) {
    if (*if_match == "*") {
      return true;
    }
    if (*if_match == etag) {
      return true;
    }
  }
  return false;
}

bool Response::IfNone(Request &request, const std::string &path) {
  if (request.HasHeader("If-None-Match") == false ||
      request.GetHeader("If-None-Match").empty()) {
    return true;
  }
  std::vector<std::string> if_none_vec = request.GetHeader("If-None-Match");
  std::string etag = GetEtag(path);
  for (std::string::iterator if_none = if_none_vec.begin();
       if_none != if_none_vec.end(); if_none++) {
    if (*if_none == "*") {
      return false;
    }
    if (*if_none == etag) {
      return false;
    }
  }
  return true;
}

bool Response::IfRange(Request &request, const std::string &path) {
  if (request.HasHeader("If-Range") == false ||
      request.GetHeader("If-Range").empty()) {
    return false;
  }
  std::string if_range_str = request.GetHeader("If-Range")[0];
  // If-Range could be a date or an ETag.
  // Try to parse as date first.
  std::time_t if_range_date = str2time(if_range_str);
  if (if_range_date != -1) {
    std::time_t last_mod = GetLastModified(path);
    return last_mod == if_range_date;
  } else {
    // Try to parse as ETag
    std::string etag = GetEtag(path);
    return etag == if_range_str;
  }
}

bool Response::FindRanges(std::vector<std::pair<size_t, size_t>> &ranges,
                          Request &request, const std::string &path) {
  if (request.HasHeader("Range") == false ||
      request.GetHeader("Range").empty()) {
    return false;
  }
  std::string range_str = request.GetHeader("Range")[0];
  // if range_str start with "bytes="
  if (range_str.find("bytes=") != 0) {
    return false;
  }
  range_str = range_str.substr(6);
  std::vector<std::string> range_vec;
  if (ws_split(range_vec, range_str, ',') <= 0) {
    return false;
  }
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return false;
  }
  for (std::vector<std::string>::iterator range = range_vec.begin();
       range != range_vec.end(); range++) {
    std::vector<std::string> range_pair;
    if (ws_split(range_pair, *range, '-') != 2) {
      return false;
    }
    std::string range_start_str = range_pair[0];
    std::string range_end_str = range_pair[1];
    if (range_start_str.empty() && range_end_str.empty()) {
      return false;
    }
    if (range_start_str.empty()) {
      range_start_str = "0";
    }
    if (range_end_str.empty()) {
      range_end_str = std::to_string(st.st_size - 1);
    }
    std::size_t range_start;
    if (ws_strtoi<size_t>(range_start, range_start_str) == false) {
      return false;
    }
    std::size_t range_end;
    if (ws_strtoi<size_t>(range_end, range_end_str) == false) {
      return false;
    }
    if (range_start > range_end) {
      return false;
    }
    if (range_end > st.st_size - 1) {
      return false;
    }
    ranges.push_back(std::make_pair(range_start, range_end));
  }
  return true;
}

std::time_t Response::GetLastModified(const std::string &path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return 0;
  }
  return st.st_mtime;
}

std::string Response::GetEtag(const std::string &path) {
  std::stringstream ss;
  ss << std::hex << file_stat.st_mtime << "-" << file_stat.st_size;
  return ss.str();
>>>>>>> 15645d4 (if~~のheaderチェックを追加)
}

bool Response::IsGetableFile(Request &request, const std::string &path) {
  // Check If headers.
  bool iIfMod = IfModSince(request, path);
  bool iIfUnmod = IfUnmodSince(request, path);
  bool iIfMatch = IfMatch(request, path);
  bool iIfNone = IfNone(request, path);
  bool iIfRange = IfRange(request, path);
  bool iFindRange = FindRanges(request, path);

  // Check to make sure any If headers are FALSE.
  // Either not-modified or no etags matched.
  if ((iIfMod == false) || (iIfNone == false)) {
    SetResponseStatus(304);
    return false;
  }
  // No matching etags or it's been modified.
  else if ((iIfMatch == false) || (iIfUnmod == false)) {
    SetResponseStatus(412);
    return false;
  }
  // Resource matched so send just the bytes requested.
  else if ((iIfRange == true) && (iFindRange == true)) {
    SetResponseStatus(206);
    // Todo: set partial
    // body(バイトレンジを指定している場合、bodyに指定された部分のみを設定する)
    return false;
  }
  // Resource didn't match, so send the entire entity.
  else {
    SetResponseStatus(200);
    return true;
  }
}

bool Response::IsDeleteableFile(Request &request, const std::string &path) {
  // Check If headers.
  bool iIfUnmod = IfUnmodSince(request, path);
  bool iIfMatch = IfMatch(request, path);
  bool iIfNone = IfNone(request, path);

  if ((iIfUnmod == FALSE) || (iIfMatch == FALSE) || (iIfNone == FALSE)) {
    SetResponseStatus(412);
    return false;
  } else {
    SetResponseStatus(200);
    return true;
  }
}

bool Response::IfModSince(Request &request, const std::string &path) {
  // If-Modified-Since
  if (request.HasHeader("If-Modified-Since") == false ||
      request.GetHeader("If-Modified-Since").empty()) {
    return true;
  }
  std::string if_mod_since_str = request.GetHeader("If-Modified-Since")[0];
  std::time_t if_mod_since = str2time(if_mod_since_str);
  if (if_mod_since == -1) {
    return true;
  }
  std::time_t last_mod = GetLastModified(path);
  return last_mod > if_mod_since;
}

bool Response::IfUnmodSince(Request &request, const std::string &path) {
  if (request.HasHeader("If-Unmodified-Since") == false ||
      request.GetHeader("If-Unmodified-Since").empty()) {
    return true;
  }
  std::string if_unmod_since_str = request.GetHeader("If-Unmodified-Since")[0];
  std::time_t if_unmod_since = str2time(if_unmod_since_str);
  if (if_unmod_since == -1) {
    return true;
  }
  std::time_t last_mod = GetLastModified(path);
  return last_mod < if_unmod_since;
}

bool Response::IfMatch(Request &request, const std::string &path) {
  if (request.HasHeader("If-Match") == false ||
      request.GetHeader("If-Match").empty()) {
    return true;
  }
  std::vector<std::string> if_match_vec = request.GetHeader("If-Match");
  std::string etag = GetEtag(path);
  for (std::string::iterator if_match = if_match_vec.begin();
       if_match != if_match_vec.end(); if_match++) {
    if (*if_match == "*") {
      return true;
    }
    if (*if_match == etag) {
      return true;
    }
  }
  return false;
}

bool Response::IfNone(Request &request, const std::string &path) {
  if (request.HasHeader("If-None-Match") == false ||
      request.GetHeader("If-None-Match").empty()) {
    return true;
  }
  std::vector<std::string> if_none_vec = request.GetHeader("If-None-Match");
  std::string etag = GetEtag(path);
  for (std::string::iterator if_none = if_none_vec.begin();
       if_none != if_none_vec.end(); if_none++) {
    if (*if_none == "*") {
      return false;
    }
    if (*if_none == etag) {
      return false;
    }
  }
  return true;
}

bool Response::IfRange(Request &request, const std::string &path) {
  if (request.HasHeader("If-Range") == false ||
      request.GetHeader("If-Range").empty()) {
    return false;
  }
  std::string if_range_str = request.GetHeader("If-Range")[0];
  // If-Range could be a date or an ETag.
  // Try to parse as date first.
  std::time_t if_range_date = str2time(if_range_str);
  if (if_range_date != -1) {
    std::time_t last_mod = GetLastModified(path);
    return last_mod == if_range_date;
  } else {
    // Try to parse as ETag
    std::string etag = GetEtag(path);
    return etag == if_range_str;
  }
}

bool Response::FindRanges(Request &request, const std::string &path) {
  if (request.HasHeader("Range") == false ||
      request.GetHeader("Range").empty()) {
    return false;
  }
  std::string range_str = request.GetHeader("Range")[0];
  // if range_str start with "bytes="
  if (range_str.find("bytes=") != 0) {
    return false;
  }
  range_str = range_str.substr(6);
  std::vector<std::string> range_vec;
  if (ws_split(range_vec, range_str, ',') <= 0) {
    return false;
  }
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return false;
  }
  for (std::vector<std::string>::iterator range = range_vec.begin();
       range != range_vec.end(); range++) {
    std::vector<std::string> range_pair;
    if (ws_split(range_pair, *range, '-') != 2) {
      return false;
    }
    std::string range_start_str = range_pair[0];
    std::string range_end_str = range_pair[1];
    if (range_start_str.empty() && range_end_str.empty()) {
      return false;
    }
    if (range_start_str.empty()) {
      range_start_str = "0";
    }
    if (range_end_str.empty()) {
      range_end_str = std::to_string(st.st_size - 1);
    }
    std::size_t range_start;
    if (ws_strtoi<size_t>(range_start, range_start_str) == false) {
      return false;
    }
    std::size_t range_end;
    if (ws_strtoi<size_t>(range_end, range_end_str) == false) {
      return false;
    }
    if (range_start > range_end) {
      return false;
    }
    if (range_end > st.st_size - 1) {
      return false;
    }
    ranges_.push_back(std::make_pair(range_start, range_end));
  }
  return true;
}

std::time_t Response::GetLastModified(const std::string &path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return 0;
  }
  return st.st_mtime;
}

std::string Response::GetEtag(const std::string &path) {
  std::stringstream ss;
  ss << std::hex << file_stat.st_mtime << "-" << file_stat.st_size;
  return ss.str();
}

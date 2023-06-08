#include "CgiResponseParser.hpp"
#include "CgiSocket.hpp"
#include "Epoll.hpp"
#include "Http.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Socket.hpp"
#include "define.hpp"
#include "utils.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <wait.h>

CgiResponseParser::CgiResponseParser(CgiSocket &cgi_socket,
                                     const Request http_request,
                                     Response &http_response)
    : cgi_socket_(cgi_socket), src_http_request_(http_request),
      dest_http_response_(http_response), redirect_type_(NO_REDIRECT) {}

CgiResponseParser::~CgiResponseParser() {}

// recv_buffer_内のCGIレスポンスからResponseを構築する
void CgiResponseParser::ParseCgiResponse() {
  std::cout << "Parse対象のCGIレスポンス" << std::endl;
  std::cout << cgi_socket_.recv_buffer_.GetString() << std::endl;

  if (ParseHeader() == FAILURE) {
    std::cerr << "ParseHeader() == FAILURE" << std::endl;
    SetInternalErrorHttpResponse();
    return;
  }
  if (ParseBody() == FAILURE) {
    std::cerr << "ParseBody() == FAILURE" << std::endl;
    SetInternalErrorHttpResponse();
    return;
  }
  if (ParseStatusHeader() == FAILURE) {
    std::cerr << "ParseStatusHeader() == FAILURE" << std::endl;
    SetInternalErrorHttpResponse();
    return;
  }
  if (ParseLocationHeader() == FAILURE) {
    std::cerr << "ParseLocationHeader() == FAILURE" << std::endl;
    SetInternalErrorHttpResponse();
    return;
  }
  if (redirect_type_ == LOCAL_CGI_REDIRECT) {
    parse_result_ = RIDIRECT_TO_LOCAL_CGI;
  } else {
    dest_http_response_ = temp_http_response_;
    dest_http_response_.SetProcessStatus(DONE);
    parse_result_ = CREATED_HTTP_RESPONSE;
  }
}

// parserメソッド郡
int CgiResponseParser::ParseHeader() {
  std::size_t header_len = 0; // ヘッダーの総合計を記録
  std::string line;
  while (cgi_socket_.recv_buffer_.GetUntilCRLF(line)) {
    if (line.empty()) {
      break;
    }
    if (!IsValidHeaderLine(line, header_len)) {
      return FAILURE;
    }
    // ヘッダーフィールドを分解して格納
    std::pair<std::string, std::string> header_pair = SplitHeader(line);
    std::vector<std::string> header_values = SplitValue(header_pair.second);
    temp_http_response_.SetHeader(header_pair.first, header_values);
  }
  return SUCCESS;
}

int CgiResponseParser::ParseBody() {
  // ボディのサイズを取得 (ボディの最後の改行２文字分を引く)
  std::size_t body_size = cgi_socket_.recv_buffer_.GetBuffSize() - 2;
  if (body_size == 0) {
    return SUCCESS;
  }
  if (IsValidBodyLength(body_size) == false) {
    std::cerr << "ボディのサイズが大きすぎます" << std::endl;
    return FAILURE;
  }

  std::string body = cgi_socket_.recv_buffer_.GetString();
  if (body[body_size] == '\r' && body[body_size + 1] == '\n') {
    // ボディの最後の改行2文字分を削除
    body.erase(body_size + 1, 2);
  } else {
    std::cerr << "ボディの最後の改行が正しくありません" << std::endl;
    return FAILURE;
  }
  temp_http_response_.SetBody(body);
  return SUCCESS;
}

int CgiResponseParser::ParseStatusHeader() {
  if (temp_http_response_.HasHeader("Status") == false) {
    temp_http_response_.SetResponseStatus(200);
    return SUCCESS;
  }

  std::vector<std::string> status_values =
      temp_http_response_.GetHeader("Status");
  if (status_values.size() != 1) {
    // "200" or "200 OK" でいずれにせよ1つのstringになるはず
    // 通常のヘッダーフィールドと違いスペースを区切り文字と扱うのでスプリットされない
    return FAILURE;
  }
  // status_values[0]をスペースで分割
  std::string status_code_str;
  std::string status_message;
  if (status_values[0].find(' ') == std::string::npos) {
    status_code_str = status_values[0];
  } else {
    status_code_str = status_values[0].substr(0, status_values[0].find(' '));
    status_message = status_values[0].substr(status_values[0].find(' ') + 1);
  }

  int status_code;
  ws_strtoi<int>(&status_code, status_code_str);
  if (IsValidStatusCode(status_code) == false) {
    return FAILURE;
  }
  if (status_message.empty()) {
    temp_http_response_.SetResponseStatus(status_code);
  } else {
    temp_http_response_.SetResponseStatus(
        Http::HttpStatus(status_code, status_message));
  }
  // HTTPのレスポンスにはStatusヘッダーは不要なので削除
  temp_http_response_.DelHeader("Status");
  return SUCCESS;
}

int CgiResponseParser::ParseLocationHeader() {
  if (temp_http_response_.HasHeader("Location") == false) {
    return SUCCESS;
  }
  std::vector<std::string> location_values =
      temp_http_response_.GetHeader("Location");
  if (ParseLocationHeaderValue(location_values) == FAILURE) {
    return FAILURE;
  }
  switch (redirect_type_) {
  case CLIENT_REDIRECT:
    // そのまま返す
    break;
  case LOCAL_STATIC_FILE_REDIRECT:
    if (GetAndSetLocalStaticFile() == FAILURE) {
      return FAILURE;
    }
    break;
  case LOCAL_CGI_REDIRECT:
    // CGI実行プロセスの作成やEPOLLへの登録は呼び出し元で行う
    if (CreateNewCgiSocketProcess() == FAILURE) {
      return FAILURE;
    }
    break;
  default:
    break;
  }
  return SUCCESS;
}

int CgiResponseParser::ParseLocationHeaderValue(
    std::vector<std::string> &location_header_value) {
  if (location_header_value.size() != 1) {
    return FAILURE;
  }
  if (location_header_value[0].find("://") != std::string::npos) {
    // 絶対URLはclient redirectなのでそのまま返す
    redirect_type_ = CLIENT_REDIRECT;
    return SUCCESS;
  }
  if (ParseLocalRedirect(location_header_value[0]) == FAILURE) {
    return FAILURE;
  }
  return SUCCESS;
}

int CgiResponseParser::ParseLocalRedirect(std::string &path) {
  if (IsValidAbsolutePath(path) == false) {
    return FAILURE;
  }

  temp_context_ = src_http_request_.GetContext();

  // fragment, query, path_info, pathを更新していく
  if (path.find('#') == std::string::npos) {
    temp_context_.resource_path.uri.fragment = "";
  } else {
    temp_context_.resource_path.uri.fragment = path.substr(path.find('#') + 1);
    path = path.substr(0, path.find('#'));
  }
  // query
  if (path.find('?') == std::string::npos) {
    temp_context_.resource_path.uri.query = "";
  } else {
    temp_context_.resource_path.uri.query = path.substr(path.find('?') + 1);
    path = path.substr(0, path.find('?'));
  }

  temp_context_.resource_path.uri.path = path;

  // root はsrc HTTPリクエストに束縛される
  std::string &root = temp_context_.location.root_;

  // pathからlocation directive以降（path_info )を除去して、rootを付与
  std::string concat =
      root + '/' + path.substr(temp_context_.location.path_.size());

  // cgi_extensionsがない場合は静的ファイルが確定する
  if (temp_context_.location.cgi_extensions_.empty()) {
    temp_context_.resource_path.server_path = concat;
    temp_context_.resource_path.path_info = ""; // path_infoは発生し得ない
    redirect_type_ = LOCAL_STATIC_FILE_REDIRECT;
    return SUCCESS;
  }

  // cgi_extensionsがある場合 リダイレクト先がCGIかどうかを確認する
  for (std::vector<std::string>::iterator ite =
           temp_context_.location.cgi_extensions_.begin();
       ite != temp_context_.location.cgi_extensions_.end(); ite++) {
    std::string cgi_extension = *ite;

    // concatの'/'ごとにextensionを確認
    for (std::string::iterator its = concat.begin(); its != concat.end();
         its++) {
      if (*its == '/') {
        std::string partial_path = concat.substr(0, its - concat.begin());
        if (ws_exist_cgi_file(partial_path, cgi_extension)) {
          temp_context_.resource_path.server_path =
              concat.substr(0, its - concat.begin());
          temp_context_.resource_path.path_info =
              concat.substr(its - concat.begin());
          redirect_type_ = LOCAL_CGI_REDIRECT;
          return SUCCESS;
        }
      }
    }
    // concatが/で終わらない場合、追加でチェックが必要
    // concatは '/'を含むので、size() > 0が保証されている
    if (concat[concat.size() - 1] != '/' &&
        ws_exist_cgi_file(concat, cgi_extension)) {
      temp_context_.resource_path.server_path = concat;
      temp_context_.resource_path.path_info = "";
      redirect_type_ = LOCAL_CGI_REDIRECT;
      return SUCCESS;
    }
  }

  // cgi_extensionsにマッチしなかった場合は静的ファイル
  temp_context_.resource_path.server_path = concat;
  temp_context_.resource_path.path_info = "";
  redirect_type_ = LOCAL_STATIC_FILE_REDIRECT;
  return SUCCESS;
}

int CgiResponseParser::GetAndSetLocalStaticFile() {
  // Open the file
  std::ifstream ifs(temp_context_.resource_path.server_path.c_str(),
                    std::ios::binary);
  if (!ifs) {
    std::cerr << "Keep Running Error: Failed to open file: "
              << temp_context_.resource_path.server_path << std::endl;
    return FAILURE;
  }

  // Get the size of the file
  std::size_t body_size = ifs.seekg(0, std::ios::end).tellg();

  // Check if the file is too large
  if (body_size > kMaxBodyLength) {
    std::cerr << "Keep Running Error: File is too large: "
              << temp_context_.resource_path.server_path << std::endl;
    return FAILURE;
  }

  // Read the file content into a string
  std::string body(body_size, '\0');
  ifs.seekg(0, std::ios::beg).read(&body[0], body.size());

  // Set HTTP response
  temp_http_response_.SetBody(body);
  temp_http_response_.SetResponseStatus(200);

  // Set Content-Type header
  temp_http_response_.SetHeader(
      "Content-Type",
      std::vector<std::string>(
          1, ws_get_mime_type(temp_context_.resource_path.server_path)));

  // Set Content-Length header
  std::stringstream ss;
  ss << body_size;
  temp_http_response_.SetHeader("Content-Length",
                                std::vector<std::string>(1, ss.str()));

  return SUCCESS;
}

int CgiResponseParser::CreateNewCgiSocketProcess() {
  Request new_request = CreateNewCgiRequest();

  redirect_new_cgi_socket_ = new CgiSocket(cgi_socket_.GetHttpClientSock(),
                                           new_request, dest_http_response_);
  cgi_socket_.GetHttpClientSock().SetCgiSocket(redirect_new_cgi_socket_);
  if (redirect_new_cgi_socket_->CreateCgiProcess() == NULL) {
    std::cerr << "Keep Running Error: Failed to create CGI process"
              << std::endl;
    return FAILURE;
  }
  return SUCCESS;
}

Request CgiResponseParser::CreateNewCgiRequest() {
  Request new_request;
  new_request.SetContext(temp_context_);
  new_request.SetMessage(src_http_request_.GetRequestMessage());
  return new_request;
}

// IsValidメソッド郡
bool CgiResponseParser::IsValidHeaderLine(const std::string &line,
                                          std::size_t &header_len) {
  if (line.empty()) {
    std::cerr << "Keep Running Error: CGI response header line is empty"
              << std::endl;
    return false;
  }
  if (!IsValidtHeaderLineLength(line)) {
    return false;
  }
  if (!IsValidHeaderLength(header_len, line.length())) {
    return false;
  }
  if (!IsValidHeaderLineFormat(line)) {
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidtHeaderLineLength(const std::string &line) {
  if (line.size() > kMaxHeaderLineLength) {
    std::cerr << "Keep Running Error: CGI response header line length too long"
              << std::endl;
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidHeaderLength(std::size_t &header_len,
                                            std::size_t line_len) {
  header_len += line_len;
  if (header_len > kMaxHeaderLength) {
    std::cerr << "Keep Running Error: CGI response header length too long"
              << std::endl;
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidHeaderLineFormat(const std::string &line) {
  // ':'の数を数えて、一つだけあることを確認
  int colon_count = std::count(line.begin(), line.end(), ':');
  if (colon_count != 1) {
    std::cerr << "Keep Running Error: CGI response header line format error"
              << std::endl;
    return false;
  }
  std::pair<std::string, std::string> kv = SplitHeader(line);
  return IsValidHeaderKey(kv.first) && IsValidHeaderValue(kv.second);
}

bool CgiResponseParser::IsValidHeaderKey(const std::string &key) {
  return IsValidToken(key);
}

bool CgiResponseParser::IsValidHeaderValue(const std::string &value) {
  if (value.empty()) {
    std::cerr << "Keep Running Error: CGI response header value is empty"
              << std::endl;
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidToken(const std::string &token) {
  if (token.empty()) {
    std::cerr << "Keep Running Error: CGI response header token is empty"
              << std::endl;
    return false;
  }
  const std::string validChars =
      "!#$%&'*+-.^_`|~"
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  if (token.find_first_not_of(validChars) != std::string::npos) {
    std::cerr << "Keep Running Error: CGI response header token format error"
              << std::endl;
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidBodyLength(std::size_t body_len) {
  if (!temp_http_response_.HasHeader("Content-Length")) {
    std::cerr << "Keep Running Error: CGI response header has no Content-Length"
              << std::endl;
    return false;
  }
  std::string content_length_value =
      temp_http_response_.GetHeader("Content-Length")[0];
  std::size_t header_content_length; // ヘッダー内のContent-Lengthの値
  ws_strtoi(&header_content_length, content_length_value);
  if (header_content_length > kMaxBodyLength) {
    std::cerr << "Keep Running Error: CGI response body length too long"
              << std::endl;
    return false;
  }
  if (body_len > header_content_length) {
    std::cerr << "Keep Running Error: CGI response body length too long"
              << std::endl;
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidStatusCode(const int status_code) {
  if (status_code < 100 || status_code > 599) {
    std::cerr << "Keep Running Error: CGI response status code error"
              << std::endl;
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidAbsolutePath(const std::string &path) {
  if (path.empty()) {
    std::cerr << "Keep Running Error: CGI response path is empty" << std::endl;
    return false;
  }
  if (path[0] != '/') {
    std::cerr << "Keep Running Error: CGI response path is not absolute"
              << std::endl;
    return false;
  }
  if (path.find("//") != std::string::npos) {
    std::cerr << "Keep Running Error: CGI response path format error"
              << std::endl;
    return false;
  }
  if (path.find("/.") != std::string::npos) {
    std::cerr << "Keep Running Error: CGI response path format error"
              << std::endl;
    return false;
  }
  if (path.find("/..") != std::string::npos) {
    std::cerr << "Keep Running Error: CGI response path format error"
              << std::endl;
    return false;
  }
  // 文字種のチェック
  const std::string validChars =
      "!#$%&'*+-.^_`|~"
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  // '?'を使ったquery stringは分割しておく
  std::string::size_type pos = path.find('?');
  std::string path_without_query_string = path.substr(0, pos);
  if (path_without_query_string.find_first_not_of(validChars) !=
      std::string::npos) {
    std::cerr << "Keep Running Error: CGI response path format error"
              << std::endl;
    return false;
  }
  return true;
}

// utilityメソッド郡
std::pair<std::string, std::string>
CgiResponseParser::SplitHeader(const std::string &headerLine) {
  std::string::size_type pos =
      headerLine.find(':'); // ':'の存在が保証されている
  std::string key = headerLine.substr(0, pos);
  std::string value = headerLine.substr(pos + 1);
  return std::make_pair(trim(key), trim(value));
}

std::vector<std::string>
CgiResponseParser::SplitValue(const std::string &value) {
  std::istringstream iss(value);
  std::string item;
  std::vector<std::string> result;
  while (std::getline(iss, item, ',')) {
    result.push_back(trim(item));
  }
  return result;
}

// 前後の空白文字(半角スペースと水平タブ)を削除するtrim関数
std::string CgiResponseParser::trim(const std::string &str) {
  const std::size_t strBegin = str.find_first_not_of(" \t");
  if (strBegin == std::string::npos)
    return ""; // 空白文字のみの場合は空文字列を返す
  const std::size_t strEnd = str.find_last_not_of(" \t");
  const std::size_t strRange = strEnd - strBegin + 1;
  return str.substr(strBegin, strRange);
}

void CgiResponseParser::SetInternalErrorHttpResponse() {
  dest_http_response_.SetResponseStatus(500);
  dest_http_response_.SetProcessStatus(DONE);
  dest_http_response_.SetVersion(
      src_http_request_.GetRequestMessage().request_line.version);
  parse_result_ = CREATED_HTTP_RESPONSE;
}

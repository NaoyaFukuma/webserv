#include "CgiResponseParser.hpp"
#include "CgiSocket.hpp"
#include "Epoll.hpp"
#include "Http.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Socket.hpp"
#include "define.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <wait.h>
#include <fstream>
#include <algorithm>

CgiResponseParser::CgiResponseParser(CgiSocket &cgi_socket)
    : cgi_socket_(cgi_socket) {
}

CgiResponseParser::~CgiResponseParser() {
}

// recv_buffer_内のCGIレスポンスからResponseを構築する
void CgiResponseParser::ParseCgiResponse() {
  std::cout << "ParseCgiResponse() start" << std::endl;
  this->http_response_.SetProcessStatus(DONE);

  size_t header_len = 0; // ヘッダーの総合計長 maxは32KB

  // HTTPリクエストからHTTP versionを取得し、response_messageに設定
  this->http_response_.SetVersion(
      this->cgi_socket_.GetRequestMessage().request_line.version);

  // HTTPリクエストのデフォルトのstatus codeを設定
  this->http_response_.SetResponseStatus(200);

  std::cout << "CGIからの出力（Parse対象）\n" << this->cgi_socket_.GetRecvBuffer().GetString() << std::endl;
  // recv_buffer_から直接1行づつ取得する
  std::string line;
  while (this->cgi_socket_.GetRecvBuffer().GetUntilCRLF(line)) {
    // 空行だったらヘッダーの終わり
    if (line.empty()) {
      break;
    }
    // 文字数、文字種、ヘッダーの総合計長をチェック
    if (!IsValidHeaderLine(line, header_len)) {
      std::cout << "Invalid Header Line" << std::endl;
      // 500 Internal Server Error
      this->http_response_.SetResponseStatus(500);
      return;
    }

    // ヘッダーフィールドを分解して格納
    std::pair<std::string, std::string> header_pair = splitHeader(line);
    std::vector<std::string> header_values =
        splitValue(header_pair.second);
    this->http_response_.SetHeader(header_pair.first, header_values);
  }

  if (this->cgi_socket_.GetRecvBuffer().GetBuffSize() != 0) {
    // ボディがあるということなので、Content-Lengthを取得する
    if (!this->http_response_.HasHeader("Content-Length")) {
      std::cout << "Content-LengthヘッダーがあるのにContent-Lengthがない" << std::endl;
      // 500 Internal Server Error ボディがあるのにContent-Lengthがない
      this->http_response_.SetResponseStatus(500);
      return;
    }
    std::vector<std::string> content_length_values =
        this->http_response_.GetHeader("Content-Length");
    size_t header_content_length; // ヘッダー内のContent-Lengthの値
    ws_strtoi(&header_content_length, content_length_values[0]);
    if (header_content_length > this->kMaxBodyLength) {
      // 500 Internal Server Error
      std::cout << "ヘッダー内のContent-Lengthが大きすぎる" << std::endl;
      this->http_response_.SetResponseStatus(500);
      return;
    }
    std::string body = this->cgi_socket_.GetRecvBuffer().GetString();
    size_t body_size = body.size();
    if (body[body_size - 1] == '\n' && body[body_size - 2] == '\r') {
      // ボディの最後の改行を削除
      body.erase(body_size - 2, 2);
    } else {
      // 500 Internal Server Error
      std::cout << "ボディの最後の改行がない" << std::endl;
      this->http_response_.SetResponseStatus(500);
      return;
    }


    if (header_content_length != body_size - 2) {
      // 500 Internal Server Error
      std::cout << "header_content_length: " << header_content_length << std::endl;
      std::cout << "body_size: " << body_size - 2 << std::endl;
      std::cout << "ヘッダー内のContent-Lengthと実際のボディの長さが一致しない" << std::endl;
      this->http_response_.SetResponseStatus(500);
      return;
    }
    this->http_response_.SetBody(this->cgi_socket_.GetRecvBuffer().GetString());
  }

  if (this->http_response_.HasHeader("Status")) {
    // Statusヘッダーがある場合は、その値を使う
    std::vector<std::string> status_values =
        this->http_response_.GetHeader("Status");
    int status_code;
    ws_strtoi(&status_code, status_values[0]);
    if (status_values.size() == 1) {
      // Statusヘッダーの値が1つ -> status code のい
      this->http_response_.SetResponseStatus(status_code);
    } else {
      // Statusヘッダーの値が2つ -> status code と status message
      this->http_response_.SetResponseStatus(Http::HttpStatus(status_code, status_values[1]));
    }
    // HTTPのレスポンスにはStatusヘッダーは不要なので削除
    this->http_response_.DelHeader("Status");
  }

  if (this->http_response_.HasHeader("Location")) {
    // Location
    // ヘッダーがある場合で、かつ、相対URLの場合は、ローカルリダイレクト
    std::vector<std::string> location_values =
        this->http_response_.GetHeader("Location");
    if (location_values[0].find("http://") == std::string::npos) {
      this->ParseLocationPath(location_values[0]);
      if (!this->cgi_socket_.GetContext().is_cgi) {
        // ローカルリダイレクト先が静的リソースなので、取得してHTTPレスポンスのボディとする。ファイルストリームで開いて
        // ファイルサイズを取得して、そのサイズ分だけ読み込む
        std::ifstream ifs(this->cgi_socket_.GetContext().resource_path.server_path.c_str(),
                          std::ios::binary);
        if (!ifs) {
          std::cout << "ローカルリダイレクト先のファイルが開けない" << std::endl;
          // 500 Internal Server Error
          this->http_response_.SetResponseStatus(500);
          return;
        }
        // sizeを取得
        size_t body_size = ifs.seekg(0, std::ios::end).tellg();
        // sizeがkMaxBodyLengthを超えていたら、500 Internal Server Error
        if (body_size > this->kMaxBodyLength) {
          std::cout << "ローカルリダイレクト先のファイルが大きすぎる" << std::endl;
          // 500 Internal Server Error
          this->http_response_.SetResponseStatus(500);
          return;
        }
        std::string body;
        body.resize(body_size);
        ifs.seekg(0, std::ios::beg).read(&body[0], body.size());
        this->http_response_.SetBody(body);
        this->http_response_.SetResponseStatus(200);
        // content-typeを設定
        this->http_response_.SetHeader(
            "Content-Type",
            std::vector<std::string>(1, ws_get_mime_type(this->cgi_socket_.GetContext().resource_path.server_path)));

        // content-lengthを設定
        std::stringstream ss;
        ss << body_size;
        this->http_response_.SetHeader("Content-Length", std::vector<std::string>(1,ss.str()));
      }
    }
  }
}

// parserメソッド郡
void CgiResponseParser::ParseLocationPath(std::string &path) {
  Context &context = this->cgi_socket_.GetContext();

  // fragment, query, pathが大元のHTTPリクエストのままなので更新
  if (path.find('#') != std::string::npos) {
    context.resource_path.uri.fragment = path.substr(path.find('#') + 1);
    path = path.substr(0, path.find('#'));
  } else {
    context.resource_path.uri.fragment = "";
  }

  if (path.find('?') !=
      std::string::npos) { // pathに'?'がある場合、クエリとして取り出しておく
    context.resource_path.uri.query = path.substr(path.find('?') + 1);
    path = path.substr(0, path.find('?'));
  } else {
    context.resource_path.uri.query = "";
  }

  context.resource_path.uri.path = path;

  // root は、HTTPリクエストに束縛される
  std::string &root = context.location.root_;

  // pathからlocation以降（path_info )を除去して、rootを付与
  std::string concat = root + '/' + path.substr(context.location.path_.size());

  // cgi_extensionsがない場合、path_infoはなく、concatをserver_pathとする
  if (context.location.cgi_extensions_.empty()) {
    context.resource_path.server_path = concat;
    context.resource_path.path_info = "";
    context.is_cgi = false;
    return;
  }

  // cgi_extensionsがある場合
  for (std::vector<std::string>::iterator ite =
           context.location.cgi_extensions_.begin();
       ite != context.location.cgi_extensions_.end(); ite++) {
    std::string cgi_extension = *ite;

    // concatの'/'ごとにextensionを確認
    for (std::string::iterator its = concat.begin(); its != concat.end();
         its++) {
      if (*its == '/') {
        std::string partial_path = concat.substr(0, its - concat.begin());
        if (ws_exist_cgi_file(partial_path, cgi_extension)) {
          context.resource_path.server_path =
              concat.substr(0, its - concat.begin());
          context.resource_path.path_info =
              concat.substr(its - concat.begin());
          context.is_cgi = true;
          return;
        }
      }
    }
    // concatが/で終わらない場合、追加でチェックが必要
    // concatは '/'を含むので、size() > 0が保証されている
    if (concat[concat.size() - 1] != '/' &&
        ws_exist_cgi_file(concat, cgi_extension)) {
      context.resource_path.server_path = concat;
      context.is_cgi = true;
      return;
    }
  }

  context.resource_path.server_path = concat;
  context.resource_path.path_info = "";
  context.is_cgi = false;
  return;
}

// IsValidメソッド郡
bool CgiResponseParser::IsValidHeaderLine(const std::string &line,
                                          size_t &header_len) {
  if (line.empty()) {
    std::cerr << "Keep Running Error: CGI response header line is empty"
              << std::endl;
    return false;
  }
  if (!IsValidtHeaderLineLength(line)) {
    return false;
  }
  if (!IsValidtHeaderLength(header_len, line.length())) {
    return false;
  }
  if (!IsValidtHeaderLineFormat(line)) {
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidtHeaderLineLength(const std::string &line) {
  if (line.size() > this->kMaxHeaderLineLength) {
    std::cerr << "Keep Running Error: CGI response header line length too long"
              << std::endl;
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidtHeaderLength(size_t &header_len,
                                             size_t line_len) {
  header_len += line_len;
  if (header_len > this->kMaxHeaderLength) {
    std::cerr << "Keep Running Error: CGI response header length too long"
              << std::endl;
    return false;
  }
  return true;
}

bool CgiResponseParser::IsValidtHeaderLineFormat(const std::string &line) {
  // ':'の数を数えて、一つだけあることを確認
  int colon_count = std::count(line.begin(), line.end(), ':');
  if (colon_count != 1) {
    std::cerr << "Keep Running Error: CGI response header line format error"
              << std::endl;
    return false;
  }
  std::pair<std::string, std::string> kv = splitHeader(line);
  return IsValidHeaderKey(kv.first) && IsValidHeaderValue(kv.second);
}

bool CgiResponseParser::IsValidHeaderKey(const std::string &key) {
  // Key should be a valid token
  return IsValidToken(key);
}

bool CgiResponseParser::IsValidHeaderValue(const std::string &value) {
  (void)value;
  // Value can contain arbitrary TEXT, so we don't need to validate it here
  return true;
}

bool CgiResponseParser::IsValidToken(const std::string &token) {
  // Token must not be empty
  if (token.empty())
    return false;

  // HTTPヘッダーフィールドのkeyで使える有効な文字種
  const std::string validChars =
      "!#$%&'*+-.^_`|~"
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  // 有効な文字種以外のポジションを返す、つまりnpos以外が返ってきたら、
  // 有効な文字種以外があるということなので、falseを返す
  return token.find_first_not_of(validChars) == std::string::npos;
}

// utilityメソッド郡
std::pair<std::string, std::string>
CgiResponseParser::splitHeader(const std::string &headerLine) {
  std::string::size_type pos = headerLine.find(':');
  if (pos != std::string::npos) {
    std::string key = headerLine.substr(0, pos);
    std::string value = headerLine.substr(pos + 1);
    return std::make_pair(trim(key), trim(value));
  } else {
    throw std::invalid_argument("Invalid header line: no ':' character");
  }
}

std::vector<std::string>
CgiResponseParser::splitValue(const std::string &value) {
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

// parseの結果を返すメソッド郡
bool CgiResponseParser::IsRedirectCgi() { return this->cgi_socket_.GetContext().is_cgi; }


Response &CgiResponseParser::GetHttpResponse() {
  return this->http_response_;
}

Request CgiResponseParser::GetHttpRequestForCgi() {
  // ローカルリダイレクト先がcgiの場合HTTPクライアントからの大元のリクエストを一部書き換えたHTTPリクエストに変換してCGIを実行する
  Request new_request;

  new_request.SetContext(this->cgi_socket_.GetContext());
  new_request.SetMessage(this->cgi_socket_.GetRequestMessage());
  return new_request;
}

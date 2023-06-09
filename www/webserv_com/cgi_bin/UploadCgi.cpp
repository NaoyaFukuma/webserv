/*
テスト用CGIを実装
POSTメソッドのエンティティボディを静的リソースとして保存する
保存先は、同じロケーションディレクトリ配下の/upload/ディレクトリ
なおこのディレクトリはルーティング対象とならず、CGIスクリプトからのみアクセス可能
保存時には、ファイル名をクエリに基づいて決定し、拡張子の前に保存の日時を追記する
GETメソッドで、クエリに基づいて/upload/ディレクトリ配下のファイルを取得する
クエリが無い場合は、/upload/ディレクトリ配下のファイル一覧を取得する
*/

#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <map>

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

std::string SetFileName(std::string query) {
  // query:を'='で分割
  std::string filename = query.substr(query.find("=") + 1);
  if (filename.empty()) {
    return "";
  }
  // 元のファイル名から拡張子を取得
  std::string ext = filename.substr(filename.find_last_of("."));
  // 拡張子を削除
  filename = filename.substr(0, filename.find_last_of("."));
  // ファイル名に日時をYYMMDDHHMMSS表記で追記。一桁の場合は0を追記
  time_t t = time(NULL);
  struct tm *local = localtime(&t);
  std::string date = std::to_string(local->tm_year + 1900);
  if (local->tm_mon + 1 < 10) {
    date = date + "0" + std::to_string(local->tm_mon + 1);
  } else {
    date = date + std::to_string(local->tm_mon + 1);
  }
  if (local->tm_mday < 10) {
    date = date + "0" + std::to_string(local->tm_mday);
  } else {
    date = date + std::to_string(local->tm_mday);
  }
  if (local->tm_hour < 10) {
    date = date + "0" + std::to_string(local->tm_hour);
  } else {
    date = date + std::to_string(local->tm_hour);
  }
  if (local->tm_min < 10) {
    date = date + "0" + std::to_string(local->tm_min);
  } else {
    date = date + std::to_string(local->tm_min);
  }
  if (local->tm_sec < 10) {
    date = date + "0" + std::to_string(local->tm_sec);
  } else {
    date = date + std::to_string(local->tm_sec);
  }
  // ファイル名に日時を追記
  filename = filename + "_" + date;
  // ファイル名に拡張子を追記
  filename = filename + ext;
  return filename;
}

std::string SetDucumentRoot() {
  std::string script_name = getenv("SCRIPT_NAME"); // /cgi-bin/UploadCgi.cgi
  std::string script_filename = getenv(
      "SCRIPT_FILENAME"); // /home/vagrant/CGI-Programming/.idea/cgi-bin/UploadCgi.cgi
  if (script_filename.empty() || script_name.empty()) {
    return "";
  }
  return script_filename.substr(
      0, script_filename.find(
             script_name)); // /home/vagrant/CGI-Programming/.idea/cgi-bin/
}

void SaveBody(std::string document_root, std::string filename) {
  std::ofstream ofs(document_root + "/upload/"+ filename); // file作成
  ofs << std::cin.rdbuf(); // UNIXドメインソケットである stdin
                           // から読み込んでファイルに書き込む
  ofs.close();
}

void ResOkFileSave(std::string filename) {
  std::stringstream ss;
  ss << "<html><head><title>OK</title></head><body>\r\n";
  ss << "<h1>OK</h1>\r\n";
  ss << "<p>File is saved as " << filename << "</p>\r\n";
  ss << "</body></html>\r\n";

  std::cout << "Status: 200 OK\r\n";
  std::cout << "Content-Length: " << ss.str().length() << "\r\n";
  std::cout << "Content-type: text/html\r\n\r\n";
  std::cout << ss.str();
}

void ResInternalError(std::string message) {
  std::stringstream ss;
  ss << "<html><head><title>Internal Server Error</title></head><body>\r\n";
  ss << "<h1>Internal Server Error</h1>\r\n";
  ss << "<p>" << message << "</p>\r\n";
  ss << "</body></html>\r\n";

  std::cout << "Status: 500 Internal Server Error\r\n";
  std::cout << "Content-Length: " << ss.str().length() << "\r\n";
  std::cout << "Content-type: text/html\r\n\r\n";
  std::cout << ss.str();
}

void ResFileList(DIR *dir) {
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

  std::cout << "Status: 200 OK\r\n";
  std::cout << "Content-Length: " << ss.str().length() << "\r\n";
  std::cout << "Content-type: text/html\r\n\r\n";
  std::cout << ss.str();
}

void ResGetFile(std::string body, std::string filename) {
  // filenameの拡張子からContent-typeを取得
  std::map<std::string, std::string> content_type_map = {
      {"html", "text/html"}, {"htm", "text/html"}, {"txt", "text/plain"},
      {"css", "text/css"},   {"png", "image/png"},  {"jpg", "image/jpeg"},
      {"jpeg", "image/jpeg"}, {"gif", "image/gif"}, {"js", "application/js"},
  };

  std::string ext = filename.substr(filename.find_last_of(".") + 1);
  std::string content_type = content_type_map[ext];
  if (content_type.empty()) { // 対応していない拡張子の場合
    content_type = "application/octet-stream";
  }

  std::cout << "Status: 200 OK\r\n";
  std::cout << "Content-Length: " << body.length() << "\r\n";
  std::cout << "Content-type: " << content_type << "\r\n\r\n";
  std::cout << body;
}

int main() {
  // クエリとMETHODを取得
  std::string query = getenv("QUERY_STRING");
  std::string method = getenv("REQUEST_METHOD");

  // POSTメソッドの場合
  if (method == "POST") {
    // クエリが無い場合は、エラーを返す
    if (query.empty()) {
      ResBadRequest("Query is empty.");
      return 0;
    }

    // クエリからファイル名設定※日時を拡張子の前に付与
    std::string filename = SetFileName(query);
    if (filename.empty()) {
      ResBadRequest("Filename is empty.");
      return 0;
    }

    // ドキュメントルートを抽出
    std::string document_root = SetDucumentRoot();
    if (document_root.empty()) {
      ResBadRequest("SCRIPT_NAME or SCRIPT_FILENAME is empty.");
    }

    // ファイルを保存
    SaveBody(document_root, filename);

    // レスポンスを返す
    ResOkFileSave(filename);
  } else if (method == "GET") {
    // クエリが無い場合は、/upload/ディレクトリ配下のファイル一覧を取得する

    // ドキュメントルートを抽出
    if (query.empty()) {
      std::string document_root = SetDucumentRoot();
      if (document_root.empty()) {
        ResBadRequest("SCRIPT_NAME or SCRIPT_FILENAME is empty.");
      }

      // ディレクトリを開く
      DIR *dir = opendir((document_root + "/upload/").c_str());
      if (dir == NULL) {
        ResBadRequest("Failed to open directory.");
        return 0;
      }

      // ファイル一覧のレスポンス
      ResFileList(dir);

      closedir(dir);
    } else {
      std::string document_root = SetDucumentRoot();
      if (document_root.empty()) {
        ResBadRequest("SCRIPT_NAME or SCRIPT_FILENAME is empty.");
      }
      std::string filename = document_root + "/upload/" + query;
      std::ifstream ifs(filename);
      if (ifs.fail()) {
        ResBadRequest("Failed to open file.");
        return 0;
      }
      std::stringstream buff;
      buff << ifs.rdbuf();
      ResGetFile(buff.str(), filename);
    }
  } else { // 不正なメソッド
    ResBadRequest("Invalid method.");
  }
}

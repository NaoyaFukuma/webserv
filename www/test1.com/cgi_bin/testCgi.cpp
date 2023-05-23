// cgi で実行されるテストプログラム
// 環境変数を全て標準出力に出力する
// 環境変数のREQUEST_METHODによってGET/POSTを判定する
// POSTの場合はCONTENT_LENGTHを元に標準入力からデータを読み込む
// HTMLのレスポンスを生成する

#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;

int main(int ac, char *av[], char *envp[]) {
  // 環境変数のREQUEST_METHODによってGET/POSTを判定する
  // POSTの場合はCONTENT_LENGTHを元に標準入力からデータを読み込む
  // HTMLのレスポンスを生成する
  // HTMLのレスポンスには環境変数をすべて含める
  const char *method = getenv("REQUEST_METHOD");
  cout
      << "Status: 200 OK\r\n"; // CGIの場合はヘッダーのStatusを出力する必要がある

  if (!method) {
    // REQUEST_METHOD が設定されていないことをHTMLで表示する
    cout << "Content-type: text/html\r\n\r\n";
    cout << "<html><head><title>testCgi</title></head><body>\r\n";
    cout << "<h1>testCgi</h1>\r\n";
    cout << "<p>REQUEST_METHOD is not set.</p>\r\n";
    cout << "<p>envp:</p>\r\n";
    cout << "<ul>\r\n";
    for (int i = 0; envp[i] != NULL; i++) {
      cout << "<li>" << envp[i] << "</li>\r\n";
    }
    cout << "</ul>\r\n";
    // acが2以上の場合はクエリ文字列を受け取っているので表示する
    if (ac >= 2) {
      cout << "<p>av:</p>\r\n";
      cout << "<ul>\r\n";
      for (int i = 0; av[i] != NULL; i++) {
        cout << "<li>" << av[i] << "</li>\r\n";
      }
      cout << "</ul>\r\n";
    }
    cout << "</body></html>\r\n";
    return 0;
  }
  string requestMethod = method;
  if (requestMethod == "POST") {
    string contentLengthStr = getenv("CONTENT_LENGTH");
    int contentLength = atoi(contentLengthStr.c_str());
    char *postData = new char[contentLength + 1];
    cin.read(postData, contentLength);
    postData[contentLength] = '\0';
    cout << "Content-type: text/html\r\n\r\n";
    cout << "<html><head><title>testCgi</title></head><body>\r\n";
    cout << "<h1>testCgi</h1>\r\n";
    cout << "<p>REQUEST_METHOD: " << requestMethod << "</p>\r\n";
    cout << "<p>CONTENT_LENGTH: " << contentLength << "</p>\r\n";
    cout << "<p>postData: " << postData << "</p>\r\n";
    cout << "<p>envp:</p>\r\n";
    cout << "<ul>\r\n";
    for (int i = 0; envp[i] != NULL; i++) {
      cout << "<li>" << envp[i] << "</li>\r\n";
    }
    cout << "</ul>\r\n";
    // acが2以上の場合はクエリ文字列を受け取っているので表示する
    if (ac >= 2) {
      cout << "<p>av:</p>\r\n";
      cout << "<ul>\r\n";
      for (int i = 0; av[i] != NULL; i++) {
        cout << "<li>" << av[i] << "</li>\r\n";
      }
      cout << "</ul>\r\n";
    }
    cout << "</body></html>\r\n";
    delete[] postData;
  } else if (requestMethod == "GET") {
    cout << "Content-type: text/html\r\n\r\n";
    cout << "<html><head><title>testCgi</title></head><body>\r\n";
    cout << "<h1>testCgi</h1>\r\n";
    cout << "<p>REQUEST_METHOD: " << requestMethod << "</p>\r\n";
    cout << "<p>envp:</p>\r\n";
    cout << "<ul>\r\n";
    for (int i = 0; envp[i] != NULL; i++) {
      cout << "<li>" << envp[i] << "</li>\r\n";
    }
    cout << "</ul>\r\n";
    // acが2以上の場合はクエリ文字列を受け取っているので表示する
    if (ac >= 2) {
      cout << "<p>av:</p>\r\n";
      cout << "<ul>\r\n";
      for (int i = 0; av[i] != NULL; i++) {
        cout << "<li>" << av[i] << "</li>\r\n";
      }
      cout << "</ul>\r\n";
    }
    cout << "</body></html>\r\n";
  } else { // 不正なリクエストメソッド
    cout
        << "Status: 200\r\n"; // CGIの場合はヘッダーのStatusを出力する必要がある
    cout << "Content-type: text/html\r\n\r\n";
    cout << "<html><head><title>testCgi</title></head><body>\r\n";
    cout << "<h1>testCgi</h1>\r\n";
    cout << "<p>REQUEST_METHOD is invalid.</p>\r\n";
    cout << "<p>envp:</p>\r\n";
    cout << "<ul>\r\n";
    for (int i = 0; envp[i] != NULL; i++) {
      cout << "<li>" << envp[i] << "</li>\r\n";
    }
    cout << "</ul>\r\n";
    // acが2以上の場合はクエリ文字列を受け取っているので表示する
    if (ac >= 2) {
      cout << "<p>av:</p>\r\n";
      cout << "<ul>\r\n";
      for (int i = 0; av[i] != NULL; i++) {
        cout << "<li>" << av[i] << "</li>\r\n";
      }
      cout << "</ul>\r\n";
    }
    cout << "</body></html>\r\n";
  }
}

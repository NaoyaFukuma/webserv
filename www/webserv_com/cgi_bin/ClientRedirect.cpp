// CGIスクリプトを実装
// クライアントリダイレクトをテストするためのもの
// Locationヘッダーフィールドを出力する
// リダイレクト先は、Googleのトップページ

#include <iostream>

int main() {
  std::cout << "Status: 302 Found\r\n";
  std::cout << "Location: https://www.google.com/\r\n";
  std::cout << "Content-type: text/html\r\n\r\n";
  return 0;
}

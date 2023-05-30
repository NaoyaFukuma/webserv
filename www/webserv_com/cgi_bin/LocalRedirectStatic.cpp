/*
CGIスクリプトを実装
ローカルリダイレクトをテストするためのもの
ローカルリダイレクト先は、同じロケーションディレクトリ配下のpublic_html/index.html
*/

#include <iostream>

int main() {
  std::cout << "Location: /public_html/index.html\r\n";
  std::cout << "Content-type: text/html\r\n\r\n";
  return 0;
}

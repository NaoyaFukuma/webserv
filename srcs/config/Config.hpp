#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <map>
#include <netinet/in.h> // sockaddr_in
#include <set>
#include <string>
#include <vector>

enum method_type {
  GET,
  POST,
  DELETE,
};

enum return_type {
  RETURN_URL,              // URLを返す
  RETURN_TEXT,             // テキストを返す
  RETURN_ONLY_STATUS_CODE, // ステータスコードのみ返す
  RETURN_EMPTY, // returnディレクティブが設定されていない
};

struct Return {
  int status_code_;
  return_type return_type_;
  std::string return_text_;
  std::string return_url_;
  std::size_t text_length_;
};

struct Location {
  Location();
  std::string path_; // locationのパス
  std::set<method_type> allow_methods_; // GET POST DELETE から１個以上指定
  uint64_t client_max_body_size_;
  // 任意 単一 デフォルト１MB, 0は無制限 制限超え 413
  // Request Entity Too Large
  // 制限されるのはボディ部分でヘッダーは含まない
  std::string root_;  // 必須 単一 相対パスの起点
  std::string index_; // 任意 単一 ディレクトリへのアクセス
  std::map<int, std::string> error_pages_;
  // 任意 複数可 mapのkeyはエラーコード, valueはエラーページのパス
  // エラーコードに重複があった場合は、最後の一つだけ保持する（上書きされる）
  bool autoindex_; // 任意 単一 デフォルトはfalse
  std::vector<std::string> cgi_extensions_; // 任意 単一
  struct Return return_;
  // 任意 単一 ステータスコードと、その時に返すファイルのパス(emptyを許容する)
};
struct Vserver { // 各バーチャルサーバーの設定を格納する
  Vserver();
  struct sockaddr_in listen_;
  // 必須 単一 複数指定された場合は最後の一つだけ保持

  std::vector<std::string> server_names_;
  // 任意 単一 ディレクティブは一つで、複数指定された場合は最後の一つだけ保持
  // 一つのディレクティブ内に、サーバーネームは並べて複数可能
  int timeout_;                     // 任意 単一 デフォルトは60秒
  std::vector<Location> locations_; // 任意 複数可
  bool
      is_default_server_; // デフォルトはfalse
                          // 一致する場合は、デフォルトサーバーとして処理する
                          // confファイルで一番上に書かれたものがデフォルトサーバー
};

typedef std::vector<Vserver> ConfVec;

class Config {
public:
  Config();
  ~Config();

  void ParseConfig(const char *src_file);
  void AddServer(Vserver &server);
  std::vector<Vserver> GetServerVec() const;

private:
  std::vector<Vserver> server_vec_; // 必須 複数可 複数の場合、一番上がデフォ
  // 不使用だが、コンパイラが自動生成し、予期せず使用するのを防ぐために記述
  Config(const Config &other);
  Config &operator=(const Config &other);
};

/*
以下のMyListenCompとConfigMapはセットで使われる。
各仮想サーバー（Vserver）の設定項目で、
その各サーバーがlistenするIPアドレスとポートの情報を持っており、
その情報が重複する可能性がある。重複することは許容されるが、
bind()するIPアドレスとポートの情報が重複するとエラーになってしまう。
従って、重複する情報を持つVserverを検出し、まとめる必要がある。
mapのkeyにstruct sockaddr_inを使い、複数のVserverをvectorで保持することで、
これらの対応関係を表現している。
*/
struct MyListenComp {
  bool operator()(const struct sockaddr_in &lhs,
                  const struct sockaddr_in &rhs) const {
    if (lhs.sin_addr.s_addr != rhs.sin_addr.s_addr) {
      return lhs.sin_addr.s_addr < rhs.sin_addr.s_addr;
    }
    return lhs.sin_port < rhs.sin_port;
  }
};

typedef std::map<struct sockaddr_in, std::vector<Vserver>, MyListenComp>
    ConfigMap;

// listen socketを作成でbind()する際にエラーにならないように、
// IPアドレスとポート番号の重複をなくしたConfigMapを作成する
ConfigMap ConfigToMap(const Config &config);

// test & debug用
std::ostream &operator<<(std::ostream &os, const Config &conf);

#endif // CONFIG_HPP_

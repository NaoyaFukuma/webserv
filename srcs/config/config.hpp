#ifndef CONF_HPP
#define CONF_HPP

#include <map>
#include <netinet/in.h> // sockaddr_in
#include <set>
#include <string>
#include <vector>

// 複数指定不可の単一のみの設定項目で、複数指定された場合は最後の一つだけ保持する
enum match_type {
  PREFIX, // 前方一致
  BACK,   // 後方一致
};

enum method_type {
  GET,
  POST,
  DELETE,
};

enum return_type {
  RETURN_URL,  // URLを返す
  RETURN_TEXT, // テキストを返す
};

struct Return {
  int status_code_;
  return_type return_type_;
  std::string return_text_;
  std::string return_url_;
  size_t text_length_;
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

struct Location {
  std::string path_; // locationのパス
  match_type match_; // 後方一致は、CGIの場合のみ使用可能
  std::set<method_type> allow_method_; // GET POST DELETE から１個以上指定
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
  bool is_cgi_; // デフォルトはfalse 一致する場合は、CGIとして処理する
  std::string cgi_path_; // execve(cgi_path, X, X) is_cgi_がtrueの場合必須
  struct Return return_;
  // 任意 単一 ステータスコードと、その時に返すファイルのパス(emptyを許容する)
};
struct Vserver { // 各バーチャルサーバーの設定を格納する
  struct sockaddr_in listen_;
  // 必須 単一 複数指定された場合は最後の一つだけ保持

  std::vector<std::string> server_names_;
  // 任意 単一 ディレクティブは一つで、複数指定された場合は最後の一つだけ保持
  // 一つのディレクティブ内に、サーバーネームは並べて複数可能
  int timeout_;                     // 任意 単一 デフォルトは60秒
  std::vector<Location> locations_; // 任意 複数可
};

class Config {
public:
  Config();
  ~Config();

  void ParseConfig(const char *src_file);
  void AddServer(const Vserver &server);
  std::vector<Vserver> GetServerVec() const;

private:
  std::vector<Vserver> server_vec_; // 必須 複数可 複数の場合、一番上がデフォ
  // 不使用だが、コンパイラが自動生成し、予期せず使用するのを防ぐために記述
  Config(const Config &other);
  Config &operator=(const Config &other);
};

// listen socketを作成でbind()する際にエラーにならないように、
// IPアドレスとポート番号の重複をなくしたConfigMapを作成する
ConfigMap ConfigToMap(const Config &config);

// debug用
std::ostream &operator<<(std::ostream &os, const Config &conf);

#endif

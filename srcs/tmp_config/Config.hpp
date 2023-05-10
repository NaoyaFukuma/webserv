#ifndef CONF_HPP
#define CONF_HPP

#include <map>
#include <netinet/in.h>
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

struct Location {
  std::string path_;
  match_type match_; // 後方一致は、CGIの場合のみ使用可能
  std::set<method_type> allow_method_; // GET POST DELETE から１個以上指定
  uint64_t max_body_size_; // 任意 単一 デフォルト１MB, 0は無制限 制限超え 413
                           // Request Entity Too Large
                           // 制限されるのはボディ部分でヘッダーは含まない
  std::string root_;               // 必須 単一 相対パスの起点
  std::vector<std::string> index_; // 任意 単一 ディレクトリへのアクセス
  bool is_cgi_; // デフォルトはfalse 一致する場合は、CGIとして処理する
  std::string cgi_path_; // execve(cgi_path, X, X) is_cgi_がtrueの場合必須
  std::map<int, std::string> error_pages_;
  // 任意 複数可 mapのkeyはエラーコード, valueはエラーページのパス
  // エラーコードに重複があった場合は、最後の一つだけ保持する（上書きされる）
  bool autoindex_; // 任意 単一 デフォルトはfalse
  std::pair<int, std::string> return_;
  // 任意 単一 ステータスコードと、その時に返すファイルのパス(emptyを許容する)
};

struct Listen { // 単一
  std::string listen_ip_port_;
  std::string listen_ip_;
  int listen_port_;
};

struct Vserver {  // 各バーチャルサーバーの設定を格納する
  Listen listen_; // 必須 単一
  std::vector<std::string> server_names_;
  // 任意 単一 ディレクティブは一つで、複数指定された場合は最後の一つだけ保持
  // 一つのディレクティブ内に、サーバーネームは並べて複数可能

  std::vector<Location> locations_; // 任意 複数可
};

typedef std::map<std::string, std::vector<Vserver> > ConfigMap;

class Config {
public:
  Config();
  ~Config();

  void AddServer(const Vserver &server);
  std::vector<Vserver> GetServerVec() const;

private:
  std::vector<Vserver> server_vec_; // 必須 複数可 複数の場合、一番上がデフォ
  // 不使用だが、コンパイラが自動生成し、予期せず使用するのを防ぐために記述
  Config(const Config &other);
  Config &operator=(const Config &other);
};

void ParseConfig(Config &dest, const char *src_file);
ConfigMap ConfigToMap(const Config &config);
std::ostream &operator<<(std::ostream &os, const Config &conf);

#endif

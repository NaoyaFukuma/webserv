#ifndef SOCKETBUFF_HPP_
#define SOCKETBUFF_HPP_

#include <sstream>
#include <string>
#include <sys/types.h>

class SocketBuff {
private:
  std::stringstream ss_;
  const static size_t kBuffSize = 1024;

public:
  SocketBuff(); // デフォルトコンストラクタ
  ~SocketBuff();

  // EPOLLINを検知したソケットFDを受け取り、全てバッファに格納する
  // SUCCESS(0): ソケット内で読み込み可能な文字をすべてバッファに格納した
  // FAILURE(-1):
  // EPOLLINが発火して一回目のrecv()で0が返った。すなわち、finパケットの受信によりEPOLLINが発火したことを示す。
  int ReadSocket(int fd);

  // バッファから1行読み込み、引数に受け取った文字列に格納する
  // 戻り値がtrueだと、lineに読み込んだ文字列が入る
  // 戻り値falseだとeofに達している
  // failbitは考慮しないものとする（文字列からstring型のため）
  // badbitの場合は、例外を投げmainでcatchして終了する
  bool GetLine(std::string &line);

  // バッファのseekgを先頭に戻す
  void ResetSeekg();
  // バッファのseekpを先頭に戻す
  void ResetSeekp();

  // バッファから任意の文字列があるか検索する 引数は検索する文字列
  // 戻り値は見つかった場合はバッファの中のその文字列の先頭のポジション、見つからない場合は-1
  // 利用のケースは、HTTPリクエストの終了”\r\n\r\n”を検索する
  ssize_t FindString(const std::string &target);

  // 引数のseekgの位置の一文字前までの文字列を返し、バッファから削除する
  // 戻り値(リクエストヘッダー)をパーサーに渡し、パーサーはstringstreamを利用してパースする
  std::string GetAndErase(const size_t pos);

  bool GetUntilCRLF(std::string &line);

  // 読み込んだ文字数を返す 現在のseekgの位置を利用している
  size_t GetReadSize();

  // 先頭から現在のseek位置までの文字数削除する
  void Erase();

  // 先頭から任意の文字数削除する 引数は削除する文字数
  void Erase(const size_t n);

  // バッファに文字列を追加する
  void AddString(const std::string &str);

  // バッファの文字列を返す
  std::string GetString();

  // バッファからソケットにノンブロッキング送信する 引数はソケットのFD
  // 返り値がfalseの場合は、全ての文字列を送信できていないので、EPOLLOUTに登録し、再度送信する
  // 送信済みの文字数はバッファから削除する
  // -1: 相手がcloseしてるなど、sendが失敗 0:送信未完了 1:送信完了
  int SendSocket(const int fd);

  // バッファの文字列をクリアする
  void ClearBuff();

  size_t GetBuffSize();

private:                                     // 使用予定無し
  SocketBuff(const SocketBuff &);            // コピーコンストラクタ
  SocketBuff &operator=(const SocketBuff &); // 代入演算子
};

#endif // SOCKETBUFF_HPP_

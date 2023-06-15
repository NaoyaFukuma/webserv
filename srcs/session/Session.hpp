#ifndef SESSION_HPP_
#define SESSION_HPP_

#include <map>

class Session {
private:
  std::map<std::time_t, int> session_map_;
  int current_max_id_;

  const static std::string kDataBasePath = "./db/session.db";

public:
  Session();
  ~Session();

  void updateSession();

private:
  // 使用予定なし
  Session(const Session &);
  Session &operator=(const Session &);
};

#endif // SESSION_HPP_

#include "Session.hpp"
#include <ctime>
#include <fstream>
#include <iostream>

Session::Session() : current_max_id_(0), session_map_() {}

Session::~Session() {}

void Session::updateSession() {
  std::time_t now = std::time(NULL);
  if (session_map_.find(now) == session_map_.end()) {
    session_map_[now] = current_max_id_;

    // kDataBasePathにkeyとvalueを新しい行に書き込む
    std::ofstream ofs;
    ofs.open(kDataBasePath, std::ios_base::app); // 追記モードで開く
    if (!ofs) {
      std::cerr << "Keep Running Error: open" << std::endl;
    } else {
      ofs << now << " " << current_max_id_++ << std::endl;
      ofs.close();
    }
  }
}

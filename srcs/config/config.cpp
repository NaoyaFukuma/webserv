#include "config.hpp"
#include "ConfigParser.hpp"
#include <iostream>

#include <arpa/inet.h> // debug用のinet_ntoa()を使うため

Config::Config() {}
Config::~Config() {}

void Config::ParseConfig(const char *src_file) {
  ConfigParser parser(src_file); // Load file
  parser.Parse(*this);
}

void Config::AddServer(Vserver &server) {
  if (this->server_vec_.empty()) {
    server.is_default_server_ = true;
  } else {
    server.is_default_server_ = false;
  }
  server_vec_.push_back(server);
}

std::vector<Vserver> Config::GetServerVec() const { return server_vec_; }

ConfigMap ConfigToMap(const Config &config) {
  ConfigMap config_map;
  std::vector<Vserver> server_vec = config.GetServerVec();
  for (size_t i = 0; i < server_vec.size(); ++i) {
    config_map[server_vec[i].listen_].push_back(server_vec[i]);
  }
  return config_map;
}

// test & debug用 Configの内容をすべて出力する
std::ostream &operator<<(std::ostream &os, const Config &conf) {
  std::vector<Vserver> server_vec = conf.GetServerVec();
  os << "-------------------------------" << '\n';

  for (size_t i = 0; i < server_vec.size(); ++i) {
    os << "server[" << i << "]:\n";
    os << "  listen: " << inet_ntoa(server_vec[i].listen_.sin_addr) << ":"
       << ntohs(server_vec[i].listen_.sin_port) << '\n';
    os << "  server_name: ";
    for (size_t j = 0; j < server_vec[i].server_names_.size(); ++j) {
      os << server_vec[i].server_names_[j] << " ";
    }
    os << '\n';
    os << "  timeout: " << server_vec[i].timeout_ << '\n';
    os << "  location: \n";
    for (size_t j = 0; j < server_vec[i].locations_.size(); ++j) {
      os << "    locations_[" << j << "]:\n";
      os << "      match: " << server_vec[i].locations_[j].match_ << '\n';
      os << "      allow_method: ";
      for (std::set<method_type>::iterator it =
               server_vec[i].locations_[j].allow_methods_.begin();
           it != server_vec[i].locations_[j].allow_methods_.end(); ++it) {
        os << *it << " ";
      }
      os << '\n';
      os << "      client_max_body_size: "
         << server_vec[i].locations_[j].client_max_body_size_ << '\n';
      os << "      root: " << server_vec[i].locations_[j].root_ << '\n';
      os << "      index: ";
      for (size_t k = 0; k < server_vec[i].locations_[j].index_.size(); ++k) {
        os << server_vec[i].locations_[j].index_[k] << " ";
      }
      os << '\n';
      os << "      is_cgi: " << server_vec[i].locations_[j].is_cgi_ << '\n';
      os << "      cgi_path: " << server_vec[i].locations_[j].cgi_path_ << '\n';
      os << "      error_pages: ";
      for (size_t k = 0; k < server_vec[i].locations_[j].error_pages_.size();
           ++k) {
        os << server_vec[i].locations_[j].error_pages_[k] << " ";
      }
      os << '\n';
      os << "      autoindex: " << server_vec[i].locations_[j].autoindex_
         << '\n';
      if (server_vec[i].locations_[j].return_.return_type_ == RETURN_EMPTY) {
        os << "      return: empty\n";
      } else if (server_vec[i].locations_[j].return_.return_type_ ==
                 RETURN_URL) {
        os << "      return: redirect\n";
        os << "              status_code: "
           << server_vec[i].locations_[j].return_.status_code_ << '\n';
        os << "      return: redirect_url: "
           << server_vec[i].locations_[j].return_.return_url_ << '\n';
      } else if (server_vec[i].locations_[j].return_.return_type_ ==
                 RETURN_TEXT) {
        os << "      return: TEXT\n";
        os << "              status_code: "
           << server_vec[i].locations_[j].return_.status_code_ << '\n';
        os << "              text: "
           << server_vec[i].locations_[j].return_.return_text_ << '\n';
      } else if (server_vec[i].locations_[j].return_.return_type_ ==
                 RETURN_ONLY_STATUS_CODE) {
        os << "      return: ONLY_STATUS_CODE\n";
        os << "              status_code: "
           << server_vec[i].locations_[j].return_.status_code_ << '\n';
      }
    }
  }
  return os;
}

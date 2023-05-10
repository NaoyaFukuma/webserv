#include "Config.hpp"
#include "ConfigParser.hpp"
#include <iostream>

Config::Config() {}
Config::~Config() {}

void Config::ParseConfig(const char *src_file) {
  ConfigParser parser(src_file); // Load file
  parser.Parse(*this);
}

void Config::AddServer(const Vserver &server) { server_vec_.push_back(server); }

std::vector<Vserver> Config::GetServerVec() const { return server_vec_; }

ConfigMap ConfigToMap(const Config &config) {
  ConfigMap config_map;
  std::vector<Vserver> server_vec = config.GetServerVec();
  for (int i = 0; i < server_vec.size(); ++i) {
    config_map[server_vec[i].listen_].push_back(server_vec[i]);
  }
  return config_map;
}

// test & debug用
std::ostream &operator<<(std::ostream &os, const Config &conf) {
  std::vector<Vserver> server_vec = conf.GetServerVec();
  if (server_vec.empty()) {
    os << "No server config" << std::endl;
    return os;
  }

  for (std::vector<Vserver>::const_iterator server_iter = server_vec.begin();
       server_iter != server_vec.end(); ++server_iter) {
    os << "server {" << std::endl;
    os << "  listen " << server_iter->listen_.listen_ip_ << ":"
       << server_iter->listen_.listen_port_ << ";" << std::endl;
    os << "  server_name";
    for (std::vector<std::string>::const_iterator name_iter =
             server_iter->server_names_.begin();
         name_iter != server_iter->server_names_.end(); ++name_iter) {
      os << " " << *name_iter;
    }
    os << ";" << std::endl;

    for (std::vector<Location>::const_iterator location_iter =
             server_iter->locations_.begin();
         location_iter != server_iter->locations_.end(); ++location_iter) {
      os << "  location " << location_iter->path_ << " {" << std::endl;
      const char *match_str[2] = {"PREFIX", "BACK"};
      os << "    match " << match_str[location_iter->match_] << ";"
         << std::endl;
      os << "    allow_method";
      const char *method_str[3] = {"GET", "POST", "DELETE"};
      for (std::set<method_type>::const_iterator method_iter =
               location_iter->allow_method_.begin();
           method_iter != location_iter->allow_method_.end(); ++method_iter) {
        os << " " << method_str[*method_iter];
      }
      os << ";" << std::endl;
      os << "    max_body_size " << location_iter->max_body_size_ << ";"
         << std::endl;
      os << "    root " << location_iter->root_ << ";" << std::endl;
      for (std::vector<std::string>::const_iterator index_iter =
               location_iter->index_.begin();
           index_iter != location_iter->index_.end(); ++index_iter) {
        os << "    index " << *index_iter << ";" << std::endl;
      }
      os << "    is_cgi " << location_iter->is_cgi_ << ";" << std::endl;
      os << "    cgi_path " << location_iter->cgi_path_ << ";" << std::endl;

      for (std::map<int, std::string>::const_iterator error_page_iter =
               location_iter->error_pages_.begin();
           error_page_iter != location_iter->error_pages_.end();
           ++error_page_iter) {
        os << "    error_page " << error_page_iter->first << " "
           << error_page_iter->second << ";" << std::endl;
      }
      os << "    autoindex " << location_iter->autoindex_ << ";" << std::endl;
      os << "    return " << location_iter->return_.first << " "
         << location_iter->return_.second << ";" << std::endl;

      os << "  }" << std::endl;
    }
    os << "}" << std::endl;
  }
  return os;
}

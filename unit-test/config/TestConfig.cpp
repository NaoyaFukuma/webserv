// BOOSTテストの設定
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
#include <arpa/inet.h>
#include "Config.hpp"
#include "ConfigParser.hpp"

#define NO_SERVER "../../../unit-test/config/invalid/NoServer.conf"
#define NO_READ_PERMISSION "../../../unit-test/config/invalid/NoReadPermission.conf"
#define INVALID_EXTENSION "../../../unit-test/config/invalid/InvalidExtension"

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define GRAY "\x1b[37m"
#define RESET "\x1b[0m"


/* ------------------------------ NG ------------------------------ */
// confファイルが存在しない場合
BOOST_AUTO_TEST_CASE(NoFile) {
  Config conf;
  // ConfigParser::ParserExceptionがスローされるか
  BOOST_CHECK_THROW(conf.ParseConfig("NoFile.conf"),
                    ConfigParser::ParserException);
}

// server文字列がない場合
BOOST_AUTO_TEST_CASE(NoServer) {
  Config conf;
  // ConfigParser::ParserExceptionがスローされるか
  // 問題は、throwされたときにどんなエラーメッセージが出るかまでは確認できないこと
  // メッセージを確認することもできるけど、今度は例外さえ投げられればいいからテスト自体は通ってしまう
  BOOST_CHECK_THROW(conf.ParseConfig(NO_SERVER),
                    ConfigParser::ParserException);
}

// read権限がない場合
BOOST_AUTO_TEST_CASE(NoReadPermission) {
  Config conf;
  // ConfigParser::ParserExceptionがスローされるか
  BOOST_CHECK_THROW(conf.ParseConfig(NO_READ_PERMISSION),
                    ConfigParser::ParserException);
}

// 拡張子がおかしい場合
BOOST_AUTO_TEST_CASE(InvalidExtension) {
  Config conf;
//   ConfigParser::ParserExceptionがスローされるか
  BOOST_CHECK_THROW(conf.ParseConfig(INVALID_EXTENSION),
                    ConfigParser::ParserException);
//  try {
//    conf.ParseConfig(INVALID_EXTENSION);
//  } catch (ConfigParser::ParserException &e) {
//    // 例外のメッセージが正しいか
//    std::cout << e.what() << std::endl;
//  }
}


/* ------------------------------ OK ------------------------------ */

#define SIMPLE "../../../unit-test/config/test_config_files/valid/Simple.conf"
#define MULTI_ROUTE "../../../unit-test/config/test_config_files/valid/MultipleRoute.conf"
#define COMPLEX "../../../unit-test/config/test_config_files/valid/Complex.conf"
#define MULTI_PORT "../../../unit-test/config/test_config_files/valid/MultiplePort.conf"

// 通常のconfファイル
BOOST_AUTO_TEST_CASE(Normal) {
  Config conf;
  BOOST_CHECK_NO_THROW(conf.ParseConfig(SIMPLE));
//  try {
//    conf.ParseConfig(NORMAL);
//  } catch (ConfigParser::ParserException &e) {
//    std::cout << RED << e.what() << RESET << std::endl;
//  }
}

//portが複数ある場合
BOOST_AUTO_TEST_CASE(MultiPort) {
  Config conf;
  conf.ParseConfig(MULTI_PORT);

  std::vector<Vserver> servers = conf.GetServerVec();

  // Check the number of servers
  BOOST_REQUIRE_EQUAL(servers.size(), 3);

  // Test first server
  BOOST_CHECK_EQUAL(servers[0].listen_.sin_port, htons(8080));
  BOOST_REQUIRE_EQUAL(servers[0].server_names_.size(), 1);
  BOOST_CHECK_EQUAL(servers[0].server_names_[0], "localhost");
  BOOST_REQUIRE_EQUAL(servers[0].locations_.size(), 1);
  BOOST_CHECK_EQUAL(servers[0].locations_[0].path_, "/");
  BOOST_CHECK(servers[0].locations_[0].allow_methods_.find(GET) != servers[0].locations_[0].allow_methods_.end());
  BOOST_CHECK_EQUAL(servers[0].locations_[0].root_, "/var/www/html");
  BOOST_CHECK_EQUAL(servers[0].locations_[0].index_, "index.html");

  // Test second server
  BOOST_CHECK_EQUAL(servers[1].listen_.sin_port, htons(80));
  BOOST_REQUIRE_EQUAL(servers[1].server_names_.size(), 2);
  BOOST_CHECK_EQUAL(servers[1].server_names_[0], "www.webserv.com");
  BOOST_CHECK_EQUAL(servers[1].server_names_[1], "webserv.com");
  BOOST_REQUIRE_EQUAL(servers[1].locations_.size(), 1);
  BOOST_CHECK_EQUAL(servers[1].locations_[0].path_, "/");
  BOOST_CHECK(servers[1].locations_[0].allow_methods_.find(DELETE) != servers[1].locations_[0].allow_methods_.end());
  BOOST_CHECK(servers[1].locations_[0].allow_methods_.find(GET) != servers[1].locations_[0].allow_methods_.end());
  BOOST_CHECK(servers[1].locations_[0].allow_methods_.find(POST) != servers[1].locations_[0].allow_methods_.end());
  BOOST_CHECK_EQUAL(servers[1].locations_[0].root_, "/var/www/html");
  BOOST_CHECK_EQUAL(servers[1].locations_[0].index_, "index.html");

  // Test third server
  BOOST_CHECK_EQUAL(servers[2].listen_.sin_port, htons(80));
  BOOST_REQUIRE_EQUAL(servers[2].server_names_.size(), 1);
  BOOST_CHECK_EQUAL(servers[2].server_names_[0], "localhost");
  BOOST_REQUIRE_EQUAL(servers[2].locations_.size(), 1);
  BOOST_CHECK_EQUAL(servers[2].locations_[0].path_, "/");
  BOOST_CHECK(servers[2].locations_[0].allow_methods_.find(GET) != servers[2].locations_[0].allow_methods_.end());
  BOOST_CHECK(servers[2].locations_[0].allow_methods_.find(POST) != servers[2].locations_[0].allow_methods_.end());
  BOOST_CHECK_EQUAL(servers[2].locations_[0].root_, "/var/www/html");
}

// locationが複数ある場合
BOOST_AUTO_TEST_CASE(MultiRoute) {
  Config config;
  config.ParseConfig(MULTI_ROUTE); // Replace with actual path of your config file

  std::vector<Vserver> servers = config.GetServerVec();
  Vserver server = servers[0]; // Assuming there is only one server in the config file

  BOOST_CHECK_EQUAL(server.listen_.sin_port, htons(4242));
  BOOST_REQUIRE_EQUAL(server.server_names_.size(), 1);
  BOOST_CHECK_EQUAL(server.server_names_[0], "localhost");

  BOOST_REQUIRE_EQUAL(server.locations_.size(), 2);
  Location locationA = server.locations_[0];
  Location locationB = server.locations_[1];

  BOOST_CHECK_EQUAL(locationA.path_, "/a/");
  BOOST_CHECK_EQUAL(locationA.root_, "/var/www/html");
  BOOST_REQUIRE_EQUAL(locationA.allow_methods_.size(), 1);
  BOOST_CHECK(locationA.allow_methods_.count(GET) == 1);

  BOOST_CHECK_EQUAL(locationB.path_, "/b/");
  BOOST_CHECK_EQUAL(locationB.root_, "/var/www/html");
  BOOST_REQUIRE_EQUAL(locationB.allow_methods_.size(), 2);
  BOOST_CHECK(locationB.allow_methods_.count(GET) == 1);
  BOOST_CHECK(locationB.allow_methods_.count(POST) == 1);
}

BOOST_AUTO_TEST_CASE(Complex) {
  Config config;
  config.ParseConfig(COMPLEX);

  std::vector<Vserver> servers = config.GetServerVec();

  BOOST_REQUIRE_EQUAL(servers.size(), 1);
  BOOST_CHECK_EQUAL(servers[0].server_names_[0], "example.com");
  BOOST_CHECK_EQUAL(servers[0].timeout_, 60);
  BOOST_CHECK(!servers[0].is_default_server_);

  std::vector<Location> locations = servers[0].locations_;
  BOOST_REQUIRE_EQUAL(locations.size(), 3);

  // Test / location
  /*
        location / {
        root /usr/share/nginx/html;
        index index.html;
        autoindex on;
        client_max_body_size 2048;
        allow_methods GET POST DELETE;
        match_type PREFIX;
        return 200;
    }
   */
  BOOST_CHECK_EQUAL(locations[0].path_, "/");
  BOOST_CHECK_EQUAL(locations[0].match_, PREFIX);
  BOOST_CHECK(locations[0].allow_methods_.find(GET) != locations[0].allow_methods_.end());
  BOOST_CHECK(locations[0].allow_methods_.find(POST) != locations[0].allow_methods_.end());
  BOOST_CHECK(locations[0].allow_methods_.find(DELETE) != locations[0].allow_methods_.end());
  BOOST_CHECK_EQUAL(locations[0].client_max_body_size_, 2048);
  BOOST_CHECK_EQUAL(locations[0].root_, "/usr/share/nginx/html");
  BOOST_CHECK_EQUAL(locations[0].index_, "index.html");
  BOOST_CHECK(locations[0].autoindex_);
  BOOST_CHECK(!locations[0].is_cgi_);
  BOOST_CHECK_EQUAL(locations[0].return_.status_code_, 200);
  BOOST_CHECK_EQUAL(locations[0].return_.return_type_, RETURN_ONLY_STATUS_CODE);

  // Test /cgi-bin/ location
  /*
        location /cgi-bin/ {
        root /usr/share/nginx/cgi-bin;
        index index.cgi;
        autoindex off;
        client_max_body_size 0;
        allow_methods GET POST;
        match_type SUFFIX;
        is_cgi true;
        cgi_path /usr/lib/cgi-bin;
        return 200;
    }
   */
    BOOST_CHECK_EQUAL(locations[1].path_, "/cgi-bin/");
    BOOST_CHECK_EQUAL(locations[1].match_, SUFFIX);
    BOOST_CHECK(locations[1].allow_methods_.find(GET) != locations[1].allow_methods_.end());
    BOOST_CHECK(locations[1].allow_methods_.find(POST) != locations[1].allow_methods_.end());
    BOOST_CHECK_EQUAL(locations[1].client_max_body_size_, 0);
    BOOST_CHECK_EQUAL(locations[1].root_, "/usr/share/nginx/cgi-bin");
    BOOST_CHECK_EQUAL(locations[1].index_, "index.cgi");
    BOOST_CHECK(!locations[1].autoindex_);
    BOOST_CHECK(locations[1].is_cgi_);
    BOOST_CHECK_EQUAL(locations[1].cgi_path_, "/usr/lib/cgi-bin");
    BOOST_CHECK_EQUAL(locations[1].return_.status_code_, 200);
    BOOST_CHECK_EQUAL(locations[1].return_.return_type_, RETURN_ONLY_STATUS_CODE);

  // Test /error/ location
  /*
        location /error/ {
        root /usr/share/nginx/error;
        index index.html;
        autoindex off;
        client_max_body_size 2048;
        allow_methods GET;
        match_type PREFIX;
        error_pages 404 /404.html;
        error_pages 500 /500.html;
        return 404;
    }
   */
    BOOST_CHECK_EQUAL(locations[2].path_, "/error/");
    BOOST_CHECK_EQUAL(locations[2].match_, PREFIX);
    BOOST_CHECK(locations[2].allow_methods_.find(GET) != locations[2].allow_methods_.end());
    BOOST_CHECK_EQUAL(locations[2].client_max_body_size_, 2048);
    BOOST_CHECK_EQUAL(locations[2].root_, "/usr/share/nginx/error");
    BOOST_CHECK_EQUAL(locations[2].index_, "index.html");
    BOOST_CHECK(!locations[2].autoindex_);
    BOOST_CHECK(!locations[2].is_cgi_);
    BOOST_CHECK_EQUAL(locations[2].error_pages_.size(), 2);
    BOOST_CHECK_EQUAL(locations[2].error_pages_[404], "/404.html");
    BOOST_CHECK_EQUAL(locations[2].error_pages_[500], "/500.html");
    BOOST_CHECK_EQUAL(locations[2].return_.status_code_, 404);
    BOOST_CHECK_EQUAL(locations[2].return_.return_type_, RETURN_ONLY_STATUS_CODE);
}

//BOOST_AUTO_TEST_CASE(ConfigToMapTest) {
//  Config config;
//  config.ParseConfig("TestVServer.conf");
//
//  ConfigMap configMap = ConfigToMap(config);
//
//  BOOST_REQUIRE_EQUAL(configMap.size(), 1);
//  // ... 同様に続ける ...
//}

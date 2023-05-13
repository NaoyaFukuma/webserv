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
#define MULTI_HOST "../../../unit-test/config/test_config_files/valid/MultipleHost.conf"

// 通常のconfファイル
BOOST_AUTO_TEST_CASE(Normal) {
  Config conf;
  BOOST_CHECK_NO_THROW(conf.ParseConfig(SIMPLE));
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

  BOOST_REQUIRE_EQUAL(servers.size(), 1);

  Vserver server = servers[0];

  // Check server
  BOOST_CHECK_EQUAL(server.listen_.sin_port, htons(4242));
  BOOST_REQUIRE_EQUAL(server.server_names_.size(), 1);
  BOOST_CHECK_EQUAL(server.server_names_[0], "localhost");
  BOOST_REQUIRE_EQUAL(server.locations_.size(), 2);
  BOOST_CHECK_EQUAL(server.is_default_server_, true);

  // Check location /a/
  Location location1 = server.locations_[0];
  BOOST_CHECK_EQUAL(location1.path_, "/a/");
  BOOST_CHECK_EQUAL(location1.root_, "/var/www/html");
  BOOST_REQUIRE_EQUAL(location1.allow_methods_.size(), 1);
  BOOST_CHECK_EQUAL(location1.allow_methods_.count(GET), 1); // Only GET method is allowed

  // Default values for location /a/
  BOOST_CHECK_EQUAL(location1.client_max_body_size_, 1024 * 1024); // Default max body size is 1MB
  BOOST_CHECK_EQUAL(location1.autoindex_, false); // Default autoindex is false
  BOOST_CHECK_EQUAL(location1.is_cgi_, false); // Default is_cgi is false
  // Add more default checks as necessary

  // Check location /b/
  Location location2 = server.locations_[1];
  BOOST_CHECK_EQUAL(location2.path_, "/b/");
  BOOST_CHECK_EQUAL(location2.root_, "/var/www/html");
  BOOST_REQUIRE_EQUAL(location2.allow_methods_.size(), 2);
  BOOST_CHECK_EQUAL(location2.allow_methods_.count(GET), 1); // GET method is allowed
  BOOST_CHECK_EQUAL(location2.allow_methods_.count(POST), 1); // POST method is allowed

  // Default values for location /b/
  BOOST_CHECK_EQUAL(location2.client_max_body_size_, 1024 * 1024); // Default max body size is 1MB
  BOOST_CHECK_EQUAL(location2.autoindex_, false); // Default autoindex is false
  BOOST_CHECK_EQUAL(location2.is_cgi_, false); // Default is_cgi is false
  // Add more default checks as necessary
}


// server_nameが複数ある場合
BOOST_AUTO_TEST_CASE(MultiHost) {
  Config config;
  config.ParseConfig(MULTI_HOST); // Replace with actual path of your config file

  std::vector<Vserver> servers = config.GetServerVec();

  BOOST_REQUIRE_EQUAL(servers.size(), 2);

  Vserver server1 = servers[0];
  Vserver server2 = servers[1];

  // Check server 1
  BOOST_CHECK_EQUAL(server1.listen_.sin_port, htons(80));
  BOOST_REQUIRE_EQUAL(server1.server_names_.size(), 1);
  BOOST_CHECK_EQUAL(server1.server_names_[0], "example.com");
  BOOST_REQUIRE_EQUAL(server1.locations_.size(), 1);
  BOOST_CHECK_EQUAL(server1.locations_[0].path_, "/");
  BOOST_CHECK_EQUAL(server1.locations_[0].root_, "/docs/");
  // Default values
  BOOST_CHECK_EQUAL(server1.timeout_, 60); // Default timeout is 60
  BOOST_CHECK_EQUAL(server1.is_default_server_, true); // Default server is false
  BOOST_CHECK_EQUAL(server1.locations_[0].client_max_body_size_, 1024 * 1024); // Default max body size is 1MB
  BOOST_CHECK_EQUAL(server1.locations_[0].autoindex_, false); // Default autoindex is false
  BOOST_CHECK_EQUAL(server1.locations_[0].is_cgi_, false); // Default is_cgi is false
  // Add more default checks as necessary

  // Check server 2
  BOOST_CHECK_EQUAL(server2.listen_.sin_port, htons(80));
  BOOST_REQUIRE_EQUAL(server2.server_names_.size(), 1);
  BOOST_CHECK_EQUAL(server2.server_names_[0], "webserv.com");
  BOOST_REQUIRE_EQUAL(server2.locations_.size(), 1);
  BOOST_CHECK_EQUAL(server2.locations_[0].path_, "/");
  BOOST_CHECK_EQUAL(server2.locations_[0].root_, "/var/www/html");
  // Default values
  BOOST_CHECK_EQUAL(server2.timeout_, 60); // Default timeout is 60
  BOOST_CHECK_EQUAL(server2.is_default_server_, false); // Default server is false
  BOOST_CHECK_EQUAL(server2.locations_[0].client_max_body_size_, 1024 * 1024); // Default max body size is 1MB
  BOOST_CHECK_EQUAL(server2.locations_[0].autoindex_, false); // Default autoindex is false
  BOOST_CHECK_EQUAL(server2.locations_[0].is_cgi_, false); // Default is_cgi is false
  // Add more default checks as necessary
}

// 色々なのが混ざったテスト
BOOST_AUTO_TEST_CASE(Complex) {
  Config config;
  try {
    config.ParseConfig(COMPLEX); // Replace with actual path of your config file
  } catch (ConfigParser::ParserException &e) {
    std::cerr << e.what() << std::endl;
  }

  std::vector<Vserver> servers = config.GetServerVec();

  BOOST_REQUIRE_EQUAL(servers.size(), 3);

  Vserver server1 = servers[0];
  Vserver server2 = servers[1];
  Vserver server3 = servers[2];

  // Check server 1
  // Check server 1
  BOOST_CHECK_EQUAL(server1.listen_.sin_port, htons(80));
  BOOST_REQUIRE_EQUAL(server1.server_names_.size(), 1);
  BOOST_CHECK_EQUAL(server1.server_names_[0], "www.example.com");
  BOOST_REQUIRE_EQUAL(server1.timeout_, 60);
  BOOST_REQUIRE_EQUAL(server1.locations_.size(), 3);

// Check each location of server 1
// Location 1
  BOOST_CHECK_EQUAL(server1.locations_[0].path_, "/");
  BOOST_CHECK_EQUAL(server1.locations_[0].root_, "/var/www/html");
  BOOST_CHECK_EQUAL(server1.locations_[0].index_, "index.html");
  BOOST_CHECK(server1.locations_[0].allow_methods_.count(GET));
  BOOST_CHECK(server1.locations_[0].allow_methods_.count(POST));
  BOOST_CHECK_EQUAL(server1.locations_[0].client_max_body_size_, 10 * 1024 * 1024);
  BOOST_CHECK_EQUAL(server1.locations_[0].autoindex_, false);
  BOOST_CHECK_EQUAL(server1.locations_[0].is_cgi_, false);

// Location 2
  BOOST_CHECK_EQUAL(server1.locations_[1].path_, "/images/");
  BOOST_CHECK_EQUAL(server1.locations_[1].root_, "/var/www/html");
  BOOST_CHECK_EQUAL(server1.locations_[1].index_, "index.html");
  BOOST_CHECK(server1.locations_[1].allow_methods_.count(GET));
  BOOST_CHECK_EQUAL(server1.locations_[1].client_max_body_size_, 0);
  BOOST_CHECK_EQUAL(server1.locations_[1].autoindex_, true);
  BOOST_CHECK_EQUAL(server1.locations_[1].is_cgi_, false);

// Location 3
  BOOST_CHECK_EQUAL(server1.locations_[2].path_, "/api/");
  BOOST_CHECK_EQUAL(server1.locations_[2].root_, "/var/www/html");
  BOOST_CHECK_EQUAL(server1.locations_[2].index_, "index.html");
  BOOST_CHECK(server1.locations_[2].allow_methods_.count(GET));
  BOOST_CHECK(server1.locations_[2].allow_methods_.count(POST));
  BOOST_CHECK(server1.locations_[2].allow_methods_.count(DELETE));
  BOOST_CHECK_EQUAL(server1.locations_[2].client_max_body_size_, 0);
  BOOST_CHECK_EQUAL(server1.locations_[2].autoindex_, false);
  BOOST_CHECK_EQUAL(server1.locations_[2].is_cgi_, true);
  BOOST_CHECK_EQUAL(server1.locations_[2].cgi_path_, "/path/to/cgi/script");


  // Check server 2
  BOOST_CHECK_EQUAL(server2.listen_.sin_port, htons(8080));
  BOOST_REQUIRE_EQUAL(server2.server_names_.size(), 1);
  BOOST_CHECK_EQUAL(server2.server_names_[0], "www.webserv.com");
  BOOST_REQUIRE_EQUAL(server2.timeout_, 120);
  BOOST_REQUIRE_EQUAL(server2.locations_.size(), 3);

// Check each location of server 2
// Location 1
  BOOST_CHECK_EQUAL(server2.locations_[0].path_, "/");
  BOOST_CHECK_EQUAL(server2.locations_[0].root_, "/var/www/html");
  BOOST_CHECK_EQUAL(server2.locations_[0].index_, "index.html");
  BOOST_CHECK(server2.locations_[0].allow_methods_.count(GET));
  BOOST_CHECK(server2.locations_[0].allow_methods_.count(POST));
  BOOST_CHECK(server2.locations_[0].allow_methods_.count(DELETE));
  BOOST_CHECK_EQUAL(server2.locations_[0].client_max_body_size_, 0);
  BOOST_CHECK_EQUAL(server2.locations_[0].autoindex_, false);
  BOOST_CHECK_EQUAL(server2.locations_[0].is_cgi_, false);

// Location 2
  BOOST_CHECK_EQUAL(server2.locations_[1].path_, "/docs/");
  BOOST_CHECK_EQUAL(server2.locations_[1].root_, "/var/www/docs");
  BOOST_CHECK_EQUAL(server2.locations_[1].index_, "index.html");
  BOOST_CHECK(server2.locations_[1].allow_methods_.count(GET));
  BOOST_CHECK_EQUAL(server2.locations_[1].client_max_body_size_, 0);
  BOOST_CHECK_EQUAL(server2.locations_[1].autoindex_, true);
  BOOST_CHECK_EQUAL(server2.locations_[1].is_cgi_, false);

// Location 3
  BOOST_CHECK_EQUAL(server2.locations_[2].path_, "/api/");
  BOOST_CHECK_EQUAL(server2.locations_[2].root_, "/var/www/html");
  BOOST_CHECK_EQUAL(server2.locations_[2].index_, "index.html");
  BOOST_CHECK(server2.locations_[2].allow_methods_.count(GET));
  BOOST_CHECK(server2.locations_[2].allow_methods_.count(POST));
  BOOST_CHECK(server2.locations_[2].allow_methods_.count(DELETE));
  BOOST_CHECK_EQUAL(server2.locations_[2].client_max_body_size_, 0);
  BOOST_CHECK_EQUAL(server2.locations_[2].autoindex_, false);
  BOOST_CHECK_EQUAL(server2.locations_[2].is_cgi_, true);
  BOOST_CHECK_EQUAL(server2.locations_[2].cgi_path_, "/path/to/cgi/script");

// Check server 3
  BOOST_CHECK_EQUAL(server3.listen_.sin_port, htons(4242));
  BOOST_REQUIRE_EQUAL(server3.server_names_.size(), 1);
  BOOST_CHECK_EQUAL(server3.server_names_[0], "localhost");
  BOOST_REQUIRE_EQUAL(server3.timeout_, 30);
  BOOST_REQUIRE_EQUAL(server3.locations_.size(), 2);

// Check each location of server 3
// Location 1
  BOOST_CHECK_EQUAL(server3.locations_[0].path_, "/");
  BOOST_CHECK_EQUAL(server3.locations_[0].root_, "/var/www/html");
  BOOST_CHECK_EQUAL(server3.locations_[0].index_, "index.html");
  BOOST_CHECK(server3.locations_[0].allow_methods_.count(GET));
  BOOST_CHECK(server3.locations_[0].allow_methods_.count(POST));
  BOOST_CHECK_EQUAL(server3.locations_[0].client_max_body_size_, 10 * 1024 * 1024);
  BOOST_CHECK_EQUAL(server3.locations_[0].autoindex_, true);
  BOOST_CHECK_EQUAL(server3.locations_[0].is_cgi_, false);

// Location 2
  BOOST_CHECK_EQUAL(server3.locations_[1].path_, "/files/");
  BOOST_CHECK_EQUAL(server3.locations_[1].root_, "/var/www/files");
  BOOST_CHECK_EQUAL(server3.locations_[1].index_, "index.html");
  BOOST_CHECK(server3.locations_[1].allow_methods_.count(GET));
  BOOST_CHECK_EQUAL(server3.locations_[1].client_max_body_size_, 0);
  BOOST_CHECK_EQUAL(server3.locations_[1].autoindex_, true);
  BOOST_CHECK_EQUAL(server3.locations_[1].is_cgi_, false);
}

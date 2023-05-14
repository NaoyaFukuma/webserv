// BOOSTテストの設定
#define BOOST_TEST_DYN_LINK

#include <arpa/inet.h>
#include "Config.hpp"
#include "ConfigParser.hpp"

/* ------------------------------ OK ------------------------------ */

#define SIMPLE "../../../unit-test/config/test_config_files/valid/Simple.conf"
#define MULTI_ROUTE "../../../unit-test/config/test_config_files/valid/MultipleRoute.conf"
#define COMPLEX_TEST "../../../unit-test/config/test_config_files/valid/Complex.conf"
#define MULTI_PORT "../../../unit-test/config/test_config_files/valid/MultiplePort.conf"
#define MULTI_HOST "../../../unit-test/config/test_config_files/valid/MultipleHost.conf"

#define BOOST_TEST_MODULE TestSimple
#include <boost/test/included/unit_test.hpp>

// 通常のconfファイル
BOOST_AUTO_TEST_CASE(Simple) {
  Config conf;
  BOOST_CHECK_NO_THROW(conf.ParseConfig(SIMPLE));
}

/* ################################################################### */

#define BOOST_TEST_MODULE TestMultiPort
#include <boost/test/included/unit_test.hpp>

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

/* ################################################################### */

#define BOOST_TEST_MODULE TestMultiRoute
#include <boost/test/included/unit_test.hpp>

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

/* ################################################################### */

#define BOOST_TEST_MODULE TestMultiHost
#include <boost/test/included/unit_test.hpp>

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

/* ################################################################### */

#define BOOST_TEST_MODULE TestComplex
#include <boost/test/included/unit_test.hpp>

// 色々なのが混ざったテスト
BOOST_AUTO_TEST_CASE(Complex) {
  Config config;
  try {
    config.ParseConfig(COMPLEX_TEST); // Replace with actual path of your config file
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

/* ################################################################### */

#define BOOST_TEST_MODULE TestOneLine
#include <boost/test/included/unit_test.hpp>

#define ONE_LINE "../../../unit-test/config/test_config_files/valid/OneLine.conf"

BOOST_AUTO_TEST_CASE(OneLine) {
  Config config;
  config.ParseConfig(ONE_LINE);

  std::vector<Vserver> servers = config.GetServerVec();

//   Check that there is exactly one server
  BOOST_TEST( servers.size() == 1 );

  Vserver server = servers[0];

  // Check server properties
  BOOST_TEST( server.listen_.sin_port == htons(8080) ); // Convert to network byte order
  BOOST_TEST( server.server_names_[0] == "localhost" );
  BOOST_TEST( server.timeout_ == 60 ); // Default timeout value
  BOOST_TEST( server.is_default_server_ == true ); // Default value

  // Check that there is exactly one location
  BOOST_TEST( server.locations_.size() == 1 );

  Location location = server.locations_[0];

  // Check location properties
  BOOST_TEST( location.path_ == "/" );
  BOOST_TEST( location.match_ == PREFIX ); // Default match type
  BOOST_TEST( location.allow_methods_.count(GET) == 1 );
  BOOST_TEST( location.client_max_body_size_ == 1024 * 1024 ); // Default value 1MB
  BOOST_TEST( location.root_ == "/var/www/html" );
  BOOST_TEST( location.index_ == "index.html" );
  BOOST_TEST( location.error_pages_[404] == "NotFound.html" );
  BOOST_TEST( location.error_pages_[403] == "NotFound.html" );
  BOOST_TEST( location.autoindex_ == false ); // Default value
  BOOST_TEST( location.is_cgi_ == false ); // Default value
//  BOOST_TEST( location.return_.status_code_ == 0 ); // Default value
  BOOST_TEST( location.return_.return_type_ == RETURN_EMPTY ); // Default value
}
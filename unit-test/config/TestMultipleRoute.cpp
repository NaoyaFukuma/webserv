#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <arpa/inet.h>
#include "Config.hpp"
#include "ConfigParser.hpp"
#define MULTI_ROUTE "../../config/test_config_files/valid/MultipleRoute.conf"

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
  BOOST_CHECK_EQUAL(location1.path_, "/");
  BOOST_CHECK_EQUAL(location1.root_, "/var/www/html");
  BOOST_REQUIRE_EQUAL(location1.allow_methods_.size(), 1);
  BOOST_CHECK_EQUAL(location1.allow_methods_.count(GET), 1); // Only GET method is allowed

  // Default values for location /a/
  BOOST_CHECK_EQUAL(location1.client_max_body_size_, 1024 * 1024); // Default max body size is 1MB
  BOOST_CHECK_EQUAL(location1.autoindex_, false); // Default autoindex is false
  //BOOST_CHECK_EQUAL(location1.is_cgi_, false); // Default is_cgi is false
  // Add more default checks as necessary

  // Check location /b/
  Location location2 = server.locations_[1];
  BOOST_CHECK_EQUAL(location2.path_, "/");
  BOOST_CHECK_EQUAL(location2.root_, "/var/www/html");
  BOOST_REQUIRE_EQUAL(location2.allow_methods_.size(), 2);
  BOOST_CHECK_EQUAL(location2.allow_methods_.count(GET), 1); // GET method is allowed
  BOOST_CHECK_EQUAL(location2.allow_methods_.count(POST), 1); // POST method is allowed

  // Default values for location /b/
  BOOST_CHECK_EQUAL(location2.client_max_body_size_, 1024 * 1024); // Default max body size is 1MB
  BOOST_CHECK_EQUAL(location2.autoindex_, false); // Default autoindex is false
  //BOOST_CHECK_EQUAL(location2.is_cgi_, false); // Default is_cgi is false
  // Add more default checks as necessary
}
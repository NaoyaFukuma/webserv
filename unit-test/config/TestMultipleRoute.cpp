#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include "Config.hpp"
#include "ConfigParser.hpp"
#include <arpa/inet.h>
#include <boost/test/included/unit_test.hpp>
#include <iostream>

#define MULTI_ROUTE "../../config/test_config_files/valid/MultipleRoute.conf"

BOOST_AUTO_TEST_CASE(MultiRoute) {
  Config config;
  config.ParseConfig(
      MULTI_ROUTE); // Replace with actual path of your config file

  std::vector<Vserver> servers = config.GetServerVec();

  BOOST_REQUIRE_EQUAL(servers.size(), 1);

  Vserver server = servers[0];

  // Check server
  BOOST_CHECK_EQUAL(server.listen_.sin_port, htons(4242));
  BOOST_REQUIRE_EQUAL(server.server_names_.size(), 1);
  BOOST_CHECK_EQUAL(server.server_names_[0], "localhost");
  BOOST_REQUIRE_EQUAL(server.locations_.size(), 3);
  BOOST_CHECK_EQUAL(server.is_default_server_, true);

  // Check location /
  Location location1 = server.locations_[0];
  BOOST_CHECK_EQUAL(location1.path_, "/");
  BOOST_CHECK_EQUAL(location1.root_, "/var/www/html");
  BOOST_REQUIRE_EQUAL(location1.allow_methods_.size(), 1);
  BOOST_CHECK_EQUAL(location1.allow_methods_.count(GET),
                    1); // Only GET method is allowed

  // Default values for location /
  BOOST_CHECK_EQUAL(location1.client_max_body_size_,
                    1024 * 1024); // Default max body size is 1MB
  BOOST_CHECK_EQUAL(location1.autoindex_, false); // Default autoindex is false
  // BOOST_CHECK_EQUAL(location1.is_cgi_, false); // Default is_cgi is false
  //  Add more default checks as necessary

  // Check location /a/
  Location location2 = server.locations_[1];
  BOOST_CHECK_EQUAL(location2.path_, "/a/");
  BOOST_CHECK_EQUAL(location2.root_, "/var/www/html/a");
  BOOST_REQUIRE_EQUAL(location2.allow_methods_.size(), 1);
  BOOST_CHECK_EQUAL(location2.allow_methods_.count(GET),
                    1); // GET method is allowed

  // Default values for location /b/
  BOOST_CHECK_EQUAL(location2.client_max_body_size_,
                    1024 * 1024); // Default max body size is 1MB
  BOOST_CHECK_EQUAL(location2.autoindex_, false); // Default autoindex is false
  // BOOST_CHECK_EQUAL(location2.is_cgi_, false); // Default is_cgi is false
  //  Add more default checks as necessary

  // Check location /b/
  Location location3 = server.locations_[2];
  BOOST_CHECK_EQUAL(location3.path_, "/b/");
  BOOST_CHECK_EQUAL(location3.root_, "/var/www/html/b");
  BOOST_REQUIRE_EQUAL(location3.allow_methods_.size(), 2);
  BOOST_CHECK_EQUAL(location3.allow_methods_.count(GET),
                    1); // GET method is allowed
  BOOST_CHECK_EQUAL(location3.allow_methods_.count(POST),
                    1); // GET method is allowed

  // Default values for location /b/
  BOOST_CHECK_EQUAL(location3.client_max_body_size_,
                    1024 * 1024); // Default max body size is 1MB
  BOOST_CHECK_EQUAL(location3.autoindex_, false); // Default autoindex is false
  // BOOST_CHECK_EQUAL(location2.is_cgi_, false); // Default is_cgi is false
  //  Add more default checks as necessary
}
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <arpa/inet.h>
#include "Config.hpp"
#include "ConfigParser.hpp"

#define MULTI_HOST "../../config/test_config_files/valid/MultipleHost.conf"

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
  //BOOST_CHECK_EQUAL(server1.locations_[0].is_cgi_, false); // Default is_cgi is false
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
  //BOOST_CHECK_EQUAL(server2.locations_[0].is_cgi_, false); // Default is_cgi is false
  // Add more default checks as necessary
}
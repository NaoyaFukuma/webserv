#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <arpa/inet.h>
#include "Config.hpp"
#include "ConfigParser.hpp"

#define MULTI_PORT "../../config/test_config_files/valid/MultiplePort.conf"

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
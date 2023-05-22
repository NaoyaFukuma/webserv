#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <arpa/inet.h>
#include "Config.hpp"
#include "ConfigParser.hpp"

#define COMPLEX "../../config/test_config_files/valid/Complex.conf"

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
  //BOOST_CHECK_EQUAL(server1.locations_[0].is_cgi_, false);

// Location 2
  BOOST_CHECK_EQUAL(server1.locations_[1].path_, "/images/");
  BOOST_CHECK_EQUAL(server1.locations_[1].root_, "/var/www/html");
  BOOST_CHECK_EQUAL(server1.locations_[1].index_, "index.html");
  BOOST_CHECK(server1.locations_[1].allow_methods_.count(GET));
  BOOST_CHECK_EQUAL(server1.locations_[1].client_max_body_size_, 0);
  BOOST_CHECK_EQUAL(server1.locations_[1].autoindex_, true);
  //BOOST_CHECK_EQUAL(server1.locations_[1].is_cgi_, false);

// Location 3
  BOOST_CHECK_EQUAL(server1.locations_[2].path_, "/api/");
  BOOST_CHECK_EQUAL(server1.locations_[2].root_, "/var/www/html");
  BOOST_CHECK_EQUAL(server1.locations_[2].index_, "index.html");
  BOOST_CHECK(server1.locations_[2].allow_methods_.count(GET));
  BOOST_CHECK(server1.locations_[2].allow_methods_.count(POST));
  BOOST_CHECK(server1.locations_[2].allow_methods_.count(DELETE));
  BOOST_CHECK_EQUAL(server1.locations_[2].client_max_body_size_, 0);
  BOOST_CHECK_EQUAL(server1.locations_[2].autoindex_, false);
  //BOOST_CHECK_EQUAL(server1.locations_[2].is_cgi_, true);
  //BOOST_CHECK_EQUAL(server1.locations_[2].cgi_path_, "/path/to/cgi/script");


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
  //BOOST_CHECK_EQUAL(server2.locations_[0].is_cgi_, false);

// Location 2
  BOOST_CHECK_EQUAL(server2.locations_[1].path_, "/docs/");
  BOOST_CHECK_EQUAL(server2.locations_[1].root_, "/var/www/docs");
  BOOST_CHECK_EQUAL(server2.locations_[1].index_, "index.html");
  BOOST_CHECK(server2.locations_[1].allow_methods_.count(GET));
  BOOST_CHECK_EQUAL(server2.locations_[1].client_max_body_size_, 0);
  BOOST_CHECK_EQUAL(server2.locations_[1].autoindex_, true);
  //BOOST_CHECK_EQUAL(server2.locations_[1].is_cgi_, false);

// Location 3
  BOOST_CHECK_EQUAL(server2.locations_[2].path_, "/api/");
  BOOST_CHECK_EQUAL(server2.locations_[2].root_, "/var/www/html");
  BOOST_CHECK_EQUAL(server2.locations_[2].index_, "index.html");
  BOOST_CHECK(server2.locations_[2].allow_methods_.count(GET));
  BOOST_CHECK(server2.locations_[2].allow_methods_.count(POST));
  BOOST_CHECK(server2.locations_[2].allow_methods_.count(DELETE));
  BOOST_CHECK_EQUAL(server2.locations_[2].client_max_body_size_, 0);
  BOOST_CHECK_EQUAL(server2.locations_[2].autoindex_, false);
  //BOOST_CHECK_EQUAL(server2.locations_[2].is_cgi_, true);
  //BOOST_CHECK_EQUAL(server2.locations_[2].cgi_path_, "/path/to/cgi/script");

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
  //BOOST_CHECK_EQUAL(server3.locations_[0].is_cgi_, false);

// Location 2
  BOOST_CHECK_EQUAL(server3.locations_[1].path_, "/files/");
  BOOST_CHECK_EQUAL(server3.locations_[1].root_, "/var/www/files");
  BOOST_CHECK_EQUAL(server3.locations_[1].index_, "index.html");
  BOOST_CHECK(server3.locations_[1].allow_methods_.count(GET));
  BOOST_CHECK_EQUAL(server3.locations_[1].client_max_body_size_, 0);
  BOOST_CHECK_EQUAL(server3.locations_[1].autoindex_, true);
  //BOOST_CHECK_EQUAL(server3.locations_[1].is_cgi_, false);
}
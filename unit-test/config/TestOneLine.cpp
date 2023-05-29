#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <arpa/inet.h>
#include "Config.hpp"
#include "ConfigParser.hpp"

#define ONE_LINE "../../config/test_config_files/valid/OneLine.conf"

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
  BOOST_TEST( location.allow_methods_.count(GET) == 1 );
  BOOST_TEST( location.client_max_body_size_ == 1024 * 1024 ); // Default value 1MB
  BOOST_TEST( location.root_ == "/var/www/html" );
  BOOST_TEST( location.index_ == "index.html" );
  BOOST_TEST( location.error_pages_[404] == "NotFound.html" );
  BOOST_TEST( location.error_pages_[403] == "NotFound.html" );
  BOOST_TEST( location.autoindex_ == false ); // Default value
  //BOOST_TEST( location.is_cgi_ == false ); // Default value
//  BOOST_TEST( location.return_.status_code_ == 0 ); // Default value
  BOOST_TEST( location.return_.return_type_ == RETURN_EMPTY ); // Default value
}

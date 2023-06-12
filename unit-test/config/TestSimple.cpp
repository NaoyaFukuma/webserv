#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <arpa/inet.h>
#include "Config.hpp"
#include "ConfigParser.hpp"

#define SIMPLE "../../test_config_files/valid/Simple.conf"

// 通常のconfファイル
BOOST_AUTO_TEST_CASE(Simple) {
  Config conf;
  BOOST_CHECK_NO_THROW(conf.ParseConfig(SIMPLE));
}
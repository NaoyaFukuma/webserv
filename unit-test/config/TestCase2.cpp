//
// Created by chanma on 2023/05/14.
//
#define BOOST_TEST_DYN_LINK

#include <arpa/inet.h>
#include "Config.hpp"
#include "ConfigParser.hpp"

#define CASE2 "../../unit-test/config/test_config_files/custom/case2.conf"

/* ================================== CASE2 ================================== */

#define BOOST_TEST_MODULE TestCase2
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(Case2) {
  boost::unit_test::unit_test_log.set_threshold_level(boost::unit_test::log_messages);

  Config conf;
  try {
    conf.ParseConfig(CASE2);
    BOOST_FAIL("Expected ParserException to be thrown but it was not.");
  } catch (const ConfigParser::ParserException& e) {
    BOOST_TEST_MESSAGE("Caught expected exception: " << e.what());
    BOOST_CHECK(true);
  } catch (...) {
    BOOST_FAIL("Caught unexpected exception.");
  }
}
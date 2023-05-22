//
// Created by chanma on 2023/05/15.
//
#define BOOST_TEST_DYN_LINK

#include "Config.hpp"
#include "ConfigParser.hpp"
#include <arpa/inet.h>

#define CASE1 "../../config/test_config_files/custom/case1.conf"

/* ================================== CASE1 ==================================
 */

#define BOOST_TEST_MODULE TestCase1
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(Case1) {
  boost::unit_test::unit_test_log.set_threshold_level(
      boost::unit_test::log_messages);

  Config conf;
  try {
    conf.ParseConfig(CASE1);
    BOOST_FAIL("Expected ParserException to be thrown but it was not.");
  } catch (const ConfigParser::ParserException &e) {
    BOOST_TEST_MESSAGE("Caught expected exception: " << e.what());
    BOOST_CHECK(true);
  } catch (...) {
    BOOST_FAIL("Caught unexpected exception.");
  }
}
#define BOOST_TEST_MODULE HelloWorldTest
#include "hello.hpp"
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_hello) {
  BOOST_CHECK_EQUAL(hello(), "Hello, World!");
}
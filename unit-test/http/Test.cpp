//
// Created by chanma on 2023/05/19.
//

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <iostream>

#include "Request.hpp"

//// リクエストメッセージのdefine
//#define REQUEST_MESSAGE R"(GET /index.html HTTP/1.1
//Host: www.example.com
//User-Agent: Mozilla/5.0
//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
//Accept-Language: en-us,en;q=0.5
//Accept-Encoding: gzip,deflate
//Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7
//Keep-Alive: 300
//Connection: keep-alive
//)"

BOOST_AUTO_TEST_CASE(Test1) {
  Request test1;
  test1.ParseHeader("Header: Value1,Value2,Value3");

  const auto& header_map = test1.GetHeaderMap();  // Assuming you have a GetHeaderMap method.
  BOOST_REQUIRE(header_map.count("Header") == 1);  // Ensure there's one entry for "Header".

  const auto& values = header_map.at("Header");
  BOOST_REQUIRE(values.size() == 3);  // Ensure there are 3 values.
  BOOST_CHECK_EQUAL(values[0], "Value1");  // Check each value.
  BOOST_CHECK_EQUAL(values[1], "Value2");
  BOOST_CHECK_EQUAL(values[2], "Value3");
}


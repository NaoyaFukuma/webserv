//
// Created by chanma on 2023/05/19.
//

#define DEBUG
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <iostream>

//#include "Request.hpp"
#include "../../srcs/http/Request.hpp"

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

  Header header_map = test1.GetHeaderMap();
  BOOST_REQUIRE(header_map.count("Header") == 1);  // Ensure there's one entry for "Header".

  std::vector<std::string> values = header_map.at("Header");
  BOOST_REQUIRE(values.size() == 3);  // Ensure there are 3 values.
  std::cout << "value[0]: " << values[0] << std::endl;
  BOOST_CHECK_EQUAL(values[0], "Value1");  // Check each value.
  std::cout << "value[1]: " << values[1] << std::endl;
  BOOST_CHECK_EQUAL(values[1], "Value2");
  std::cout << "value[2]: " << values[2] << std::endl;
  BOOST_CHECK_EQUAL(values[2], "Value3");
}


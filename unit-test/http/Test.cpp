//
// Created by chanma on 2023/05/19.
//

#define DEBUG
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <iostream>

#include "Request.hpp"
#include "SocketBuff.hpp"

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

BOOST_AUTO_TEST_CASE(ParseHeader) {
  Request test;
  test.ParseHeader("Header:          \t Value1,Value2,Value3");

  Header header_map = test.GetHeaderMap();
  std::cout << "header_map.size(): " << header_map.size() << std::endl;
  BOOST_REQUIRE(header_map.count("Header") == 1);  // Ensure there's one entry for "Header".

  std::vector<std::string> values = header_map.at("Header");
  std::cout << "values.size(): " << values.size() << std::endl;
  BOOST_REQUIRE(values.size() == 3);  // Ensure there are 3 values.
  std::cout << "value[0]: " << values[0] << std::endl;
  BOOST_CHECK_EQUAL(values[0], "Value1");  // Check each value.
  std::cout << "value[1]: " << values[1] << std::endl;
  BOOST_CHECK_EQUAL(values[1], "Value2");
  std::cout << "value[2]: " << values[2] << std::endl;
  BOOST_CHECK_EQUAL(values[2], "Value3");
}

BOOST_AUTO_TEST_CASE(Status) {
  { /* 1 */
    Request test;

    test.SetParseStatus(HEADER);
    test.ParseHeader("\r\n");
    ParseStatus parse_status = test.GetParseStatus();
    BOOST_CHECK_EQUAL(parse_status, BODY);
  }
  { /* 2 */
    Request test;

    test.SetParseStatus(HEADER);
    test.ParseHeader("Header:          \t Value1,Value2,Value3\r\n");
    ParseStatus parse_status = test.GetParseStatus();
    BOOST_CHECK_EQUAL(parse_status, HEADER);
  }
}

BOOST_AUTO_TEST_CASE(Body) {
//  Request test;
//
//    test.SetParseStatus(BODY);
  // GetWordで一番初めから切り取る
}

BOOST_AUTO_TEST_CASE(BodyType) {
  { /* 1 */
    Request test;

    test.SetParseStatus(HEADER);
    BOOST_CHECK_EQUAL(test.GetChunkStatus(), -1);
    test.ParseHeader("Transfer-Encoding: chunked\r\n");
    if (!test.JudgeBodyType()) {
      BOOST_FAIL("1: JudgeBodyType failed");
    }
    BOOST_CHECK_EQUAL(test.GetChunkStatus(), true);
  }
  { /* 2 */
    Request test;
    const static long long CONTENT_LENGTH = 100;

    test.SetParseStatus(HEADER);
    test.ParseHeader("Content-Length: 100\r\n");
    if (!test.JudgeBodyType()) {
      BOOST_FAIL("2: JudgeBodyType failed");
    }
    BOOST_CHECK_EQUAL(test.GetContentLength(), CONTENT_LENGTH);
  }
}

// 81文字のリクエストヘッダ
BOOST_AUTO_TEST_CASE(ParseContentLengthBody) {
  Request test;

  test.SetParseStatus(HEADER);
  test.ParseHeader("Content-Length: 81\r\n");
  test.SetParseStatus(BODY);
  if (!test.JudgeBodyType()) {
    BOOST_FAIL("2: JudgeBodyType failed");
  }

  std::string REQUEST_BODY = \
R"({
  "firstName": "John",
  "lastName": "Doe",
  "email": "john.doe@example.com"
})";

  BOOST_TEST_MESSAGE(REQUEST_BODY);
  BOOST_CHECK_EQUAL(test.GetContentLength(), 81);
  test.ParseBody(REQUEST_BODY);
  std::cout << "test.GetBody(): " << test.GetBody() << std::endl;
  BOOST_CHECK_EQUAL(test.GetBody(), REQUEST_BODY);
}

#include <boost/assign/list_of.hpp>

#define REQUEST "POST /api/v1/users HTTP/1.1\r\n" \
                "Host: www.example.com\r\n" \
                "Content-Type: application/json\r\n" \
                "Content-Length: 81\r\n" \
                "\r\n" \
                "{\r\n" \
                "  \"firstName\": \"John\",\r\n" \
                "  \"lastName\": \"Doe\",\r\n" \
                "  \"email\": \"john.doe@example.com\"\r\n" \
                "}\r\n"

BOOST_AUTO_TEST_CASE(Genaral) {
  SocketBuff socket_buff;
  Request test;

  socket_buff.AddString(REQUEST);
  test.Parse(socket_buff);

  // Expected values
  std::string expected_method = "POST";
  std::string expected_uri = "/api/v1/users";
  Http::Version expected_version = Http::HTTP11;
  std::string expected_body = "{\r\n  \"firstName\": \"John\",\r\n  \"lastName\": \"Doe\",\r\n  \"email\": \"john.doe@example.com\"\r\n}\r\n";
  std::vector<std::string> expected_host = boost::assign::list_of("www.example.com");
  std::vector<std::string> expected_content_type = boost::assign::list_of("application/json");
  std::vector<std::string> expected_content_length = boost::assign::list_of("81");

  // Assertions
  BOOST_CHECK_EQUAL(test.GetRequestMessage().request_line.method, expected_method);
  BOOST_CHECK_EQUAL(test.GetRequestMessage().request_line.uri, expected_uri);
  BOOST_CHECK_EQUAL(test.GetRequestMessage().request_line.version, expected_version);
  BOOST_CHECK_EQUAL(test.GetRequestMessage().body, expected_body);

  // Check each header
  Header headers = test.GetRequestMessage().header;
  Header::iterator it;
  for (it = headers.begin(); it != headers.end(); ++it) {
    if (it->first == "Host") {
      BOOST_CHECK_EQUAL_COLLECTIONS(it->second.begin(), it->second.end(), expected_host.begin(), expected_host.end());
    } else if (it->first == "Content-Type") {
      BOOST_CHECK_EQUAL_COLLECTIONS(it->second.begin(), it->second.end(), expected_content_type.begin(),
                                    expected_content_type.end());
    } else if (it->first == "Content-Length") {
      BOOST_CHECK_EQUAL_COLLECTIONS(it->second.begin(), it->second.end(), expected_content_length.begin(),
                                    expected_content_length.end());
    }
  }
}
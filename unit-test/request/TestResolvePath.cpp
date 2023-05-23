#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ResolvePath
#include "Config.hpp"
#include "Request.hpp"
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(SimpleConfig) {
  Config config;
  config.ParseConfig("../../config/test_config_files/valid/Simple.conf");
  std::cout << config << std::endl;
  Request request;
  RequestMessage message = {.request_line = {.method = "GET",
                                             .uri = "/test/index.html",
                                             .version = Http::HTTP11},
                            .header = {},
                            .body = ""};
  request.SetMessage(message);
  request.ResolvePath(config);
  BOOST_CHECK_EQUAL(request.GetContext().resource_path.server_path,
                    "/var/www/html/test/index.html");
}

BOOST_AUTO_TEST_CASE(SimpleConfigWithoutSlash) {
  Config config;
  config.ParseConfig("../../config/test_config_files/valid/Simple.conf");
  std::cout << config << std::endl;
  Request request;
  RequestMessage message = {.request_line = {.method = "GET",
                                             .uri = "test/index.html",
                                             .version = Http::HTTP11},
                            .header = {},
                            .body = ""};
  request.SetMessage(message);
  request.ResolvePath(config);
  BOOST_CHECK_EQUAL(request.GetContext().resource_path.server_path,
                    "/var/www/html/test/index.html");
}

BOOST_AUTO_TEST_CASE(MultipleLocation) {
  Config config;
  config.ParseConfig("../../config/test_config_files/valid/MultipleRoute.conf");
  std::cout << config << std::endl;
  Request request;
  RequestMessage message = {.request_line = {.method = "GET",
                                             .uri = "/b/a/index.html",
                                             .version = Http::HTTP11},
                            .header = {},
                            .body = ""};
  request.SetMessage(message);
  request.ResolvePath(config);
  BOOST_CHECK_EQUAL(request.GetContext().resource_path.server_path,
                    "/var/www/html/b/a/index.html");
}

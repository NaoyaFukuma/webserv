#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ResolvePath
#include "Config.hpp"
#include "Request.hpp"
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(SimpleConfig) {
  Config config;
  config.ParseConfig("../../config/test_config_files/valid/Simple.conf");
  Request request;
  RequestMessage message = {.request_line = {.method = "GET",
                                             .uri = "/test/index.html",
                                             .version = Http::HTTP11},
                            .header = {},
                            .body = ""};
  request.SetMessage(message);
  request.ResolvePath(config.GetServerVec());
  BOOST_CHECK_EQUAL(request.GetContext().resource_path.server_path,
                    "/var/www/html/test/index.html");
}

BOOST_AUTO_TEST_CASE(SimpleConfigWithoutSlash) {
  Config config;
  config.ParseConfig("../../config/test_config_files/valid/Simple.conf");
  Request request;
  RequestMessage message = {.request_line = {.method = "GET",
                                             .uri = "test/index.html",
                                             .version = Http::HTTP11},
                            .header = {},
                            .body = ""};
  request.SetMessage(message);
  request.ResolvePath(config.GetServerVec());
  BOOST_CHECK_EQUAL(request.GetContext().resource_path.server_path,
                    "/var/www/html/test/index.html");
}

BOOST_AUTO_TEST_CASE(MultipleLocation) {
  Config config;
  config.ParseConfig("../../config/test_config_files/valid/MultipleRoute.conf");
  Request request;
  RequestMessage message = {.request_line = {.method = "GET",
                                             .uri = "/b/a/index.html",
                                             .version = Http::HTTP11},
                            .header = {},
                            .body = ""};
  request.SetMessage(message);
  request.ResolvePath(config.GetServerVec());
  BOOST_CHECK_EQUAL(request.GetContext().resource_path.server_path,
                    "/var/www/html/b/a/index.html");
}

BOOST_AUTO_TEST_CASE(MultipleServerLocation) {
  Config config;
  config.ParseConfig("../../config/test_config_files/valid/Complex.conf");
  Request request;
  RequestMessage message = {.request_line = {.method = "GET",
                                             .uri = "/files/index.html",
                                             .version = Http::HTTP11},
                            .header = {{"Host", {"localhost"}}},
                            .body = ""};
  request.SetMessage(message);
  request.ResolvePath(config.GetServerVec());
  BOOST_CHECK_EQUAL(request.GetContext().resource_path.server_path,
                    "/var/www/files/index.html");
}

BOOST_AUTO_TEST_CASE(Cgi) {
  Config config;
  config.ParseConfig("../../request/test_config_files/cgi.conf");
  Request request;
  RequestMessage message = {.request_line = {.method = "GET",
                                             .uri = "/cgi/cgi.py",
                                             .version = Http::HTTP11},
                            .header = {{"Host", {"cgi"}}},
                            .body = ""};
  request.SetMessage(message);
  request.ResolvePath(config.GetServerVec());
  BOOST_CHECK_EQUAL(request.GetContext().resource_path.server_path,
                    "/app/unit-test/request/test_cgi_bin/cgi.py");
  BOOST_CHECK_EQUAL(request.GetContext().is_cgi, true);
}

BOOST_AUTO_TEST_CASE(PathInfo) {
  Config config;
  config.ParseConfig("../../request/test_config_files/cgi.conf");
  Request request;
  RequestMessage message = {.request_line = {.method = "GET",
                                             .uri = "/cgi/cgi.py/path/info",
                                             .version = Http::HTTP11},
                            .header = {{"Host", {"cgi"}}},
                            .body = ""};
  request.SetMessage(message);
  request.ResolvePath(config.GetServerVec());
  BOOST_CHECK_EQUAL(request.GetContext().resource_path.server_path,
                    "/app/unit-test/request/test_cgi_bin/cgi.py");
  BOOST_CHECK_EQUAL(request.GetContext().resource_path.path_info, "/path/info");
  BOOST_CHECK_EQUAL(request.GetContext().is_cgi, true);
}

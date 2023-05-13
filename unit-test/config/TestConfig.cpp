// BOOSTテストの設定
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
#include "Config.hpp"

// confファイルの拡張子がおかしい場合のテスト
BOOST_AUTO_TEST_CASE(InvalidExtension) {
  Config conf;
  BOOST_CHECK_THROW(conf.ParseConfig("unit-test/config/invalid.conf"),
                    std::runtime_error);
}
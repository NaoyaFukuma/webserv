//
// Created by chanma on 2023/05/14.
//
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <arpa/inet.h>
#include "Config.hpp"
#include "ConfigParser.hpp"

#define NO_SERVER "../../unit-test/config/invalid/NoServer.conf"
#define NO_READ_PERMISSION "../../unit-test/config/invalid/NoReadPermission.conf"
#define INVALID_EXTENSION "../../unit-test/config/invalid/InvalidExtension"

/* ------------------------------ NG ------------------------------ */
// confファイルが存在しない場合
BOOST_AUTO_TEST_CASE(NoFile) {
        Config conf;
        // ConfigParser::ParserExceptionがスローされるか
        BOOST_CHECK_THROW(conf.ParseConfig("NoFile.conf"),
        ConfigParser::ParserException);
}

// server文字列がない場合
BOOST_AUTO_TEST_CASE(NoServer) {
        Config conf;
        // ConfigParser::ParserExceptionがスローされるか
        // 問題は、throwされたときにどんなエラーメッセージが出るかまでは確認できないこと
        // メッセージを確認することもできるけど、今度は例外さえ投げられればいいからテスト自体は通ってしまう
        BOOST_CHECK_THROW(conf.ParseConfig(NO_SERVER),
        ConfigParser::ParserException);
}

// read権限がない場合
BOOST_AUTO_TEST_CASE(NoReadPermission) {
        Config conf;
        // ConfigParser::ParserExceptionがスローされるか
        BOOST_CHECK_THROW(conf.ParseConfig(NO_READ_PERMISSION),
        ConfigParser::ParserException);
}

// 拡張子がおかしい場合
BOOST_AUTO_TEST_CASE(InvalidExtension) {
        Config conf;
//   ConfigParser::ParserExceptionがスローされるか
        BOOST_CHECK_THROW(conf.ParseConfig(INVALID_EXTENSION),
        ConfigParser::ParserException);
//  try {
//    conf.ParseConfig(INVALID_EXTENSION);
//  } catch (ConfigParser::ParserException &e) {
//    // 例外のメッセージが正しいか
//    std::cout << e.what() << std::endl;
//  }
}


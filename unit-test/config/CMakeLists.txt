set(TEST_CONF_SOURCE_FILE TestConfig.cpp)

add_executable(test_config ${TEST_CONF_SOURCE_FILE})

target_link_libraries(test_config PRIVATE MyConfig Boost::unit_test_framework)

# インクルードパスの設定
target_include_directories(test_config PRIVATE ${PROJECT_SOURCE_DIR}/srcs/config)

add_test(NAME test_config COMMAND test_config)

enable_testing()
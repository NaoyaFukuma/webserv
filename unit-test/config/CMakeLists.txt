set(TEST_INVALID_FILES TestInvalidConf.cpp)
set(TEST_CONF_SOURCE_FILE TestConfig.cpp)

add_executable(test_invalid_config ${TEST_INVALID_FILES})
add_executable(test_config ${TEST_CONF_SOURCE_FILE})

# test1
target_link_libraries(test_invalid_config PRIVATE MyConfig Boost::unit_test_framework)
target_include_directories(test_invalid_config PRIVATE ${PROJECT_SOURCE_DIR}/srcs/config)
add_test(NAME test_invalid_config COMMAND test_invalid_config)
enable_testing()

# test2
target_link_libraries(test_config PRIVATE MyConfig Boost::unit_test_framework)
target_include_directories(test_config PRIVATE ${PROJECT_SOURCE_DIR}/srcs/config)
add_test(NAME test_config COMMAND test_config)
enable_testing()
set(TEST_INVALID_FILES TestInvalidConf.cpp)

add_executable(test_invalid_configs ${TEST_INVALID_FILES})

# =================================== test_invalid_configs =================================== #

target_link_libraries(test_invalid_configs PRIVATE MyConfig Boost::unit_test_framework)
target_include_directories(test_invalid_configs PRIVATE ${PROJECT_SOURCE_DIR}/srcs/config)
add_test(NAME test_invalid_configs COMMAND test_invalid_configs)
enable_testing()


# ==================================== test_valid_configs ==================================== #

# Simple_config
add_executable(simple_config TestValidConfig.cpp)
target_compile_definitions(simple_config PRIVATE BOOST_TEST_MODULE=TestSimple)
target_link_libraries(simple_config PRIVATE MyConfig Boost::unit_test_framework)
target_include_directories(simple_config PRIVATE ${PROJECT_SOURCE_DIR}/srcs/config)
add_test(NAME simple_config COMMAND simple_config)

# MultiPort_config
add_executable(multiport_config TestValidConfig.cpp)
target_compile_definitions(multiport_config PRIVATE BOOST_TEST_MODULE=TestMultiPort)
target_link_libraries(multiport_config PRIVATE MyConfig Boost::unit_test_framework)
target_include_directories(multiport_config PRIVATE ${PROJECT_SOURCE_DIR}/srcs/config)
add_test(NAME multiport_config COMMAND multiport_config)

# MultiRoute_config
add_executable(multiroute_config TestValidConfig.cpp)
target_compile_definitions(multiroute_config PRIVATE BOOST_TEST_MODULE=TestMultiRoute)
target_link_libraries(multiroute_config PRIVATE MyConfig Boost::unit_test_framework)
target_include_directories(multiroute_config PRIVATE ${PROJECT_SOURCE_DIR}/srcs/config)
add_test(NAME multiroute_config COMMAND multiroute_config)

# MultiHost_config
add_executable(multihost_config TestValidConfig.cpp)
target_compile_definitions(multihost_config PRIVATE BOOST_TEST_MODULE=TestMultiHost)
target_link_libraries(multihost_config PRIVATE MyConfig Boost::unit_test_framework)
target_include_directories(multihost_config PRIVATE ${PROJECT_SOURCE_DIR}/srcs/config)
add_test(NAME multihost_config COMMAND multihost_config)

# Complex_config
add_executable(complex_config TestValidConfig.cpp)
target_compile_definitions(complex_config PRIVATE BOOST_TEST_MODULE=TestComplex)
target_link_libraries(complex_config PRIVATE MyConfig Boost::unit_test_framework)
target_include_directories(complex_config PRIVATE ${PROJECT_SOURCE_DIR}/srcs/config)
add_test(NAME complex_config COMMAND complex_config)

# OneLine_config
add_executable(oneline_config TestValidConfig.cpp)
target_compile_definitions(oneline_config PRIVATE BOOST_TEST_MODULE=TestOneLine)
target_link_libraries(oneline_config PRIVATE MyConfig Boost::unit_test_framework)
target_include_directories(oneline_config PRIVATE ${PROJECT_SOURCE_DIR}/srcs/config)
add_test(NAME oneline_config COMMAND oneline_config)

# ==================================== test_custom_configs ==================================== #

# case1
add_executable(case1 TestCase1.cpp)
target_compile_definitions(case1 PRIVATE BOOST_TEST_MODULE=TestCase1)
target_link_libraries(case1 PRIVATE MyConfig Boost::unit_test_framework)
target_include_directories(case1 PRIVATE ${PROJECT_SOURCE_DIR}/srcs/config)
add_test(NAME case1 COMMAND case1)

# case2
add_executable(case2 TestCase2.cpp)
target_compile_definitions(case2 PRIVATE BOOST_TEST_MODULE=TestCase2)
target_link_libraries(case2 PRIVATE MyConfig Boost::unit_test_framework)
target_include_directories(case2 PRIVATE ${PROJECT_SOURCE_DIR}/srcs/config)
add_test(NAME case2 COMMAND case2)
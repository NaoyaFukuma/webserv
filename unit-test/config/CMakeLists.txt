set(TEST_INVALID_FILES TestInvalidConf.cpp)

add_executable(test_invalid_configs ${TEST_INVALID_FILES})

# =================================== test_invalid_configs =================================== #

target_link_libraries(test_invalid_configs PRIVATE MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(test_invalid_configs PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME test_invalid_configs COMMAND test_invalid_configs)
enable_testing()


# ==================================== test_valid_configs ==================================== #

# Simple_config
set(TEST_SIMPLE TestSimple.cpp)
add_executable(simple_config ${TEST_SIMPLE})
target_link_libraries(simple_config PRIVATE PRIVATE MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(simple_config PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME simple_config COMMAND simple_config)
enable_testing()

# MultiPort_config
set(TEST_MULTIPORT TestMultiplePort.cpp)
add_executable(multiport_config ${TEST_MULTIPORT})
target_link_libraries(multiport_config PRIVATE PRIVATE MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(multiport_config PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME multiport_config COMMAND multiport_config)
enable_testing()

# MultiRoute_config
set(TEST_MULTIROUTE TestMultipleRoute.cpp)
add_executable(multiroute_config ${TEST_MULTIROUTE})
target_link_libraries(multiroute_config PRIVATE PRIVATE MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(multiroute_config PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME multiroute_config COMMAND multiroute_config)
enable_testing()

# MultiHost_config
set(TEST_MULTIRHOST TestMultipleHost.cpp)
add_executable(multihost_config ${TEST_MULTIRHOST})
target_link_libraries(multihost_config PRIVATE PRIVATE MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(multihost_config PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME multihost_config COMMAND multihost_config)
enable_testing()

# Complex_config
set(TEST_COMPLEX TestMultipleHost.cpp)
add_executable(multicomplex_config ${TEST_COMPLEX})
target_link_libraries(multicomplex_config PRIVATE PRIVATE MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(multicomplex_config PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME multicomplex_config COMMAND multicomplex_config)
enable_testing()

# OneLine_config
set(TEST_ONELINE TestMultipleHost.cpp)
add_executable(oneline_config ${TEST_ONELINE})
target_link_libraries(oneline_config PRIVATE PRIVATE MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(oneline_config PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME oneline_config COMMAND oneline_config)
enable_testing()

# ==================================== test_custom_configs ==================================== #

# case1
add_executable(case1 TestCase1.cpp)
target_compile_definitions(case1 PRIVATE BOOST_TEST_MODULE=TestCase1)
target_link_libraries(case1 PRIVATE MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(case1 PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME case1 COMMAND case1)

# case2
add_executable(case2 TestCase2.cpp)
target_compile_definitions(case2 PRIVATE BOOST_TEST_MODULE=TestCase2)
target_link_libraries(case2 PRIVATE MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(case2 PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME case2 COMMAND case2)
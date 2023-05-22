set(TEST_INVALID_FILES TestInvalidConf.cpp)

add_executable(test_invalid_configs ${TEST_INVALID_FILES})

# =================================== test_invalid_configs =================================== #

target_link_libraries(test_invalid_configs PRIVATE MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(test_invalid_configs PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME test_invalid_configs COMMAND test_invalid_configs)
enable_testing()


# ==================================== test_valid_configs ==================================== #

# Simple_config
set(TEST_SIMPLE_FILES TestSimple.cpp)
add_executable(simple ${TEST_SIMPLE_FILES})
target_link_libraries(simple PRIVATE MyHttpLib MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(simple PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME simple COMMAND simple)
enable_testing()

# MultiPort_config
set(TEST_MULTIPORT_FILES TestMultiplePort.cpp)
add_executable(multiport ${TEST_MULTIPORT_FILES})
target_link_libraries(multiport PRIVATE MyHttpLib MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(multiport PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME multiport COMMAND multiport)
enable_testing()

# MultiRoute_config
set(TEST_MULTIROUTE_FILES TestMultipleRoute.cpp)
add_executable(multiroute ${TEST_MULTIROUTE_FILES})
target_link_libraries(multiroute PRIVATE MyHttpLib MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(multiroute PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME multiroute COMMAND multiroute)
enable_testing()

# MultiHost_config
set(TEST_MULTIHOST_FILES TestMultipleHost.cpp)
add_executable(multihost ${TEST_MULTIHOST_FILES})
target_link_libraries(multihost PRIVATE MyHttpLib MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(multihost PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME multihost COMMAND multihost)
enable_testing()

# Complex_config
set(TEST_COMPLEX_FILES TestComplex.cpp)
add_executable(complex ${TEST_COMPLEX_FILES})
target_link_libraries(complex PRIVATE MyHttpLib MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(complex PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME complex COMMAND complex)
enable_testing()

# OneLine_config
set(TEST_ONELINE_FILES TestOneLine.cpp)
add_executable(oneline ${TEST_ONELINE_FILES})
target_link_libraries(oneline PRIVATE MyHttpLib MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(oneline PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME oneline COMMAND oneline)
enable_testing()

# ==================================== test_custom_configs ==================================== #

# case1
set(TEST_CASE1_FILES TestCase1.cpp)
add_executable(case1 ${TEST_CASE1_FILES})
target_link_libraries(case1 PRIVATE MyHttpLib MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(case1 PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME case1 COMMAND case1)
enable_testing()

# case2
set(TEST_CASE2_FILES TestCase2.cpp)
add_executable(case2 ${TEST_CASE2_FILES})
target_link_libraries(case2 PRIVATE MyHttpLib MyConfigLib MyUtilsLib Boost::unit_test_framework)
target_include_directories(case2 PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
add_test(NAME case2 COMMAND case2)
enable_testing()
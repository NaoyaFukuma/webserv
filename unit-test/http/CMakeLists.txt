set(TEST_FILES Test.cpp)

# =================================== General  =================================== #

add_executable(general ${TEST_FILES})
target_link_libraries(general PRIVATE MyHttpLib MyUtilsLib MyServerLib Boost::unit_test_framework)
target_include_directories(general PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/http)
add_test(NAME general COMMAND general)
enable_testing()

# =================================== Status  =================================== #

add_executable(status ${TEST_FILES})
target_link_libraries(status PRIVATE MyHttpLib MyUtilsLib MyServerLib Boost::unit_test_framework)
target_include_directories(status PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/http)
add_test(NAME status COMMAND status)
enable_testing()

# =================================== Body  =================================== #

add_executable(body ${TEST_FILES})
target_link_libraries(body PRIVATE MyHttpLib MyUtilsLib MyServerLib Boost::unit_test_framework)
target_include_directories(body PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/http)
add_test(NAME body COMMAND body)
enable_testing()
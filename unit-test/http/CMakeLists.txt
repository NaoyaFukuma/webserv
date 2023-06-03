set(TEST_FILE Test.cpp)

enable_testing()

# =================================== Header =================================== #

add_executable(header ${TEST_FILE})
target_link_libraries(header PRIVATE MyHttpLib MyUtilsLib MyServerLib Boost::unit_test_framework)
target_include_directories(header PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/http)
add_test(NAME header COMMAND header)

# =================================== Status =================================== #

add_executable(status ${TEST_FILE})
target_link_libraries(status PRIVATE MyHttpLib MyUtilsLib MyServerLib Boost::unit_test_framework)
target_include_directories(status PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/http)
add_test(NAME status COMMAND status)

# =================================== Body =================================== #

add_executable(body ${TEST_FILE})
target_link_libraries(body PRIVATE MyHttpLib MyUtilsLib MyServerLib Boost::unit_test_framework)
target_include_directories(body PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/http)
add_test(NAME body COMMAND body)

add_executable(body-chunked ${TEST_FILE})
target_link_libraries(body-chunked PRIVATE MyHttpLib MyUtilsLib MyServerLib Boost::unit_test_framework)
target_include_directories(body-chunked PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/http)
add_test(NAME body-chunked COMMAND body-chunked)

# =================================== General =================================== #

add_executable(general1 ${TEST_FILE})
target_link_libraries(general1 PRIVATE MyHttpLib MyUtilsLib MyServerLib Boost::unit_test_framework)
target_include_directories(general1 PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/http)
add_test(NAME general1 COMMAND general1)

add_executable(general2 ${TEST_FILE})
target_link_libraries(general2 PRIVATE MyHttpLib MyUtilsLib MyServerLib Boost::unit_test_framework)
target_include_directories(general2 PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/http)
add_test(NAME general2 COMMAND general2)

# =================================== TestFunc =================================== #


add_executable(testfunc ${TEST_FILE})
target_link_libraries(testfunc PRIVATE MyHttpLib MyUtilsLib MyServerLib Boost::unit_test_framework)
target_include_directories(testfunc PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/http)
add_test(NAME testfunc COMMAND testfunc)
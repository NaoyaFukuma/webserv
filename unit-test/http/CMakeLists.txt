set(TEST_FILES Test.cpp)

add_executable(test1 ${TEST_FILES})

# =================================== test_invalid_configs =================================== #

target_compile_definitions(test1 PRIVATE TEST)
target_link_libraries(test1 PRIVATE MyHttpLib MyUtilsLib MyServerLib Boost::unit_test_framework)
target_include_directories(test1 PRIVATE ${PROJECT_SOURCE_DIR}/srcs/http)
add_test(NAME test1 COMMAND test1)
enable_testing()

#add_executable(test_resolve_path TestResolvePath.cpp)
#target_compile_definitions(test_resolve_path PRIVATE BOOST_TEST_MODULE=TestMultiPort)
#target_link_libraries(test_resolve_path PRIVATE MyServerLib MyConfigLib MyUtilsLib MyHttpLib Boost::unit_test_framework)
#target_include_directories(test_resolve_path PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config)
#target_include_directories(test_resolve_path PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/http)
#target_include_directories(test_resolve_path PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/server)
#add_test(NAME test_resolve_path COMMAND test_resolve_path)
#enable_testing()

set(TEST_FILE TestResolvePath.cpp)

enable_testing()

# set absolute path
get_filename_component(ABS_PATH1 ../test_config_files/valid/Simple.conf ABSOLUTE)
add_definitions(-DSIMPLECONF_PATH="${ABS_PATH1}")

get_filename_component(ABS_PATH2 ../test_config_files/valid/MultipleRoute.conf ABSOLUTE)
add_definitions(-DMULTIROUTECONF_PATH="${ABS_PATH2}")

get_filename_component(ABS_PATH3 ../test_config_files/valid/Complex.conf ABSOLUTE)
add_definitions(-DCOMPLEXCONF_PATH="${ABS_PATH3}")

# =================================== simpleconfig =================================== #

add_executable(simpleconfig ${TEST_FILE})
target_link_libraries(simpleconfig PRIVATE MyServerLib MyConfigLib MyUtilsLib MyHttpLib Boost::unit_test_framework)
target_include_directories(simpleconfig PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config ${PROJECT_SOURCE_DIR}/../srcs/http ${PROJECT_SOURCE_DIR}/../srcs/server)
add_test(NAME simpleconfig COMMAND simpleconfig)

# =================================== simpleconfigwithoutslash =================================== #

add_executable(simpleconfigwithoutslash ${TEST_FILE})
target_link_libraries(simpleconfigwithoutslash PRIVATE MyServerLib MyConfigLib MyUtilsLib MyHttpLib Boost::unit_test_framework)
target_include_directories(simpleconfigwithoutslash PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config ${PROJECT_SOURCE_DIR}/../srcs/http ${PROJECT_SOURCE_DIR}/../srcs/server)
add_test(NAME simpleconfigwithoutslash COMMAND simpleconfigwithoutslash)

# =================================== multiplelocation =================================== #

add_executable(multiplelocation ${TEST_FILE})
target_link_libraries(multiplelocation PRIVATE MyServerLib MyConfigLib MyUtilsLib MyHttpLib Boost::unit_test_framework)
target_include_directories(multiplelocation PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config ${PROJECT_SOURCE_DIR}/../srcs/http ${PROJECT_SOURCE_DIR}/../srcs/server)
add_test(NAME multiplelocation COMMAND multiplelocation)

## =================================== multipleserverlocation =================================== #
#
#add_executable(multipleserverlocation ${TEST_FILE})
#target_link_libraries(multipleserverlocation PRIVATE MyServerLib MyConfigLib MyUtilsLib MyHttpLib Boost::unit_test_framework)
#target_include_directories(multipleserverlocation PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config ${PROJECT_SOURCE_DIR}/../srcs/http ${PROJECT_SOURCE_DIR}/../srcs/server)
#add_test(NAME multipleserverlocation COMMAND multipleserverlocation)
#
## =================================== cgi =================================== #
#
#add_executable(cgi ${TEST_FILE})
#target_link_libraries(cgi PRIVATE MyServerLib MyConfigLib MyUtilsLib MyHttpLib Boost::unit_test_framework)
#target_include_directories(cgi PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config ${PROJECT_SOURCE_DIR}/../srcs/http ${PROJECT_SOURCE_DIR}/../srcs/server)
#add_test(NAME cgi COMMAND cgi)
#
## =================================== pathinfo =================================== #
#
#add_executable(pathinfo ${TEST_FILE})
#target_link_libraries(pathinfo PRIVATE MyServerLib MyConfigLib MyUtilsLib MyHttpLib Boost::unit_test_framework)
#target_include_directories(pathinfo PRIVATE ${PROJECT_SOURCE_DIR}/../srcs/config ${PROJECT_SOURCE_DIR}/../srcs/http ${PROJECT_SOURCE_DIR}/../srcs/server)
#add_test(NAME pathinfo COMMAND pathinfo)
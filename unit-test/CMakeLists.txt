cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)

# Boost
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/../srcs/config)
include_directories(${PROJECT_SOURCE_DIR}/../srcs/http)
include_directories(${PROJECT_SOURCE_DIR}/../srcs/server)
include_directories(${PROJECT_SOURCE_DIR}/../srcs/utils)

# Source files
set(CONFIG_SOURCE_FILES
        ${PROJECT_SOURCE_DIR}/../srcs/config/Config.cpp
        ${PROJECT_SOURCE_DIR}/../srcs/config/ConfigParser.cpp
        )

set(HTTP_SOURCE_FILES
        ${PROJECT_SOURCE_DIR}/../srcs/http/Http.cpp
        ${PROJECT_SOURCE_DIR}/../srcs/http/Request.cpp
        ${PROJECT_SOURCE_DIR}/../srcs/http/Response.cpp
        )

set(SERVER_SOURCE_FILES
        ${PROJECT_SOURCE_DIR}/../srcs/server/Epoll.cpp
        ${PROJECT_SOURCE_DIR}/../srcs/server/Server.cpp
        ${PROJECT_SOURCE_DIR}/../srcs/server/Socket.cpp
        ${PROJECT_SOURCE_DIR}/../srcs/server/SocketBuff.cpp
        )

set(UTILS_SOURCE_FILES
        ${PROJECT_SOURCE_DIR}/../srcs/utils/utils.cpp
        ${PROJECT_SOURCE_DIR}/../srcs/utils/utils.tpp
        )

# Create libraries
add_library(MyConfigLib ${CONFIG_SOURCE_FILES})
add_library(MyHttpLib ${HTTP_SOURCE_FILES} ${CONFIG_SOURCE_FILES})
add_library(MyServerLib ${SERVER_SOURCE_FILES})
add_library(MyUtilsLib ${UTILS_SOURCE_FILES})

# Check options for unit tests
option(BUILD_HTTP_UNIT_TESTS "Build the HTTP unit tests" ON)
option(BUILD_CONF_UNIT_TESTS "Build the config unit tests" ON)

if(BUILD_HTTP_UNIT_TESTS)
    add_subdirectory(http)
    target_compile_definitions(MyHttpLib PRIVATE DEBUG)
else()
    message(STATUS "Skipping HTTP tests")
endif()

if(BUILD_CONF_UNIT_TESTS)
    add_subdirectory(config)
else()
    message(STATUS "Skipping config tests")
endif()

if(BUILD_PATH_UNIT_TESTS)
    add_subdirectory(resolve_path)
else()
    message(STATUS "Skipping config tests")
endif()

enable_testing()
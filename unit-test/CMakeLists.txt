cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)

# Boost
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../srcs/config)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../srcs/server)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../srcs/tmp_utils)

# Source files
set(CONFIG_SOURCE_FILES
        ../srcs/config/Config.cpp
        ../srcs/config/ConfigParser.cpp
        )

set(SERVER_SOURCE_FILES
        ../srcs/server/Epoll.cpp
        ../srcs/server/Server.cpp
        ../srcs/server/Socket.cpp
        ../srcs/server/SocketBuff.cpp
        )

set(UTILS_SOURCE_FILES
        ../srcs/tmp_utils/utils.cpp
        ../srcs/tmp_utils/utils.tpp
        )

# Create libraries
add_library(MyConfigLib ${CONFIG_SOURCE_FILES})
# 指定がない場合はインクルードしない
option(BUILD_CONF_UNIT_TESTS "Build the unit tests" ON)
if(BUILD_CONF_UNIT_TESTS)
    add_library(MyServerLib ${SERVER_SOURCE_FILES})
else()
    message(STATUS "Skipping server directory for unit test build")
endif()
add_library(MyUtilsLib ${UTILS_SOURCE_FILES})

# Add subdirectory
add_subdirectory(config)

enable_testing()
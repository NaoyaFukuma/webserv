# CMakeLists.txt (project_root)
cmake_minimum_required(VERSION 3.10)

project(MyProject)

set(CMAKE_CXX_STANDARD 17)

# サブディレクトリを追加
add_subdirectory(srcs)

# Boost
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_subdirectory(unit-test)
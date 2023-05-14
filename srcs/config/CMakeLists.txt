# CMakeLists.txt (src/conf)
set(CONF_SOURCE_FILES Config.cpp ConfigParser.cpp)

# ライブラリを生成
add_library(MyConfig ${CONF_SOURCE_FILES})

# インクルードディレクトリを指定
target_link_libraries(MyConfig MyUtils)

target_include_directories(MyConfig PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
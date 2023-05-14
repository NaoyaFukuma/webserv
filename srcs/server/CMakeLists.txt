# CMakeLists.txt (src/server)
set(SERVER_SOURCE_FILES Epoll.cpp Server.cpp Socket.cpp SocketBuff.cpp)

# ライブラリを生成
add_library(MyServer ${SERVER_SOURCE_FILES})

# インクルードディレクトリを指定
target_link_libraries(MyServer MyUtils)
target_link_libraries(MyServer MyCongig)
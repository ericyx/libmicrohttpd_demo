cmake_minimum_required(VERSION 2.8)
PROJECT(libmicrohttpd_demo)

SET(CMAKE_CXX_FLAGS "-std=c++11")

include_directories(/usr/include)
include_directories(/usr/local/include)

add_executable(main main.cpp)
target_link_libraries(main fileserver_example.so /usr/local/lib/libmicrohttpd.so)

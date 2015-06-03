cmake_minimum_required(VERSION 2.8)
PROJECT(httpjson)

SET(CMAKE_CXX_FLAGS "-std=c++11")

include_directories(/usr/include)
include_directories(/usr/local/include)

add_executable(httpjson httpjson.cpp)

target_link_libraries(httpjson /usr/local/lib/libmicrohttpd.so)


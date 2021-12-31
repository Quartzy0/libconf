cmake_minimum_required(VERSION 3.16.0)

project(libconf C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

add_executable(libconf src/main.c src/libconf.c)

target_include_directories(libconf PRIVATE include)
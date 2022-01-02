cmake_minimum_required(VERSION 3.16.0)

project(libconf C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")

add_executable(libconf src/main.c src/libconf.c src/timer.h)

target_include_directories(libconf PRIVATE include)
cmake_minimum_required(VERSION 3.16.0)

project(libconf C)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")

add_library(libconf src/libconf.c)
target_include_directories(libconf PRIVATE include)

project(libconf_test C)

add_executable(libconf_test test/main.c test/timer.h)

target_link_libraries(libconf_test libconf)
target_include_directories(libconf_test PRIVATE include)


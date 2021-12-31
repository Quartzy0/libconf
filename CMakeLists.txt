cmake_minimum_required(VERSION 3.16.0)

project(libconf C)

add_executable(libconf src/main.c src/libconf_m.c src/libconf_s.c)

target_include_directories(libconf PRIVATE include)
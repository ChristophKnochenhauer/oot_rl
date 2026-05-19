cmake_minimum_required(VERSION 3.20)
project(oot_rl LANGUAGES CXX)
add_executable(hello src/hello.cpp)

add_subdirectory(external/Shipwright EXCLUDE_FROM_ALL)

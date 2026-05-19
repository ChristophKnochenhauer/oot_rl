cmake_minimum_required(VERSION 3.20)
project(oot_rl LANGUAGES CXX)

add_subdirectory(external/Shipwright EXCLUDE_FROM_ALL)

add_executable(hello src/hello.cpp)

target_link_libraries(hello PRIVATE libultraship)

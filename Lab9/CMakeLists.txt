cmake_minimum_required(VERSION 3.17)
project(Lab9 C)

set(CMAKE_C_STANDARD 99)

add_executable(Lab9 main.c)

target_link_libraries(Lab9 pthread)
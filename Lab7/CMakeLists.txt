cmake_minimum_required(VERSION 3.17)
project(Lab7 C)

set(CMAKE_C_STANDARD 99)

add_executable(Lab7 main.c)

target_link_libraries(Lab7 pthread)
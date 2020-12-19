cmake_minimum_required(VERSION 3.16.5)
project(Lab8 C)

set(CMAKE_C_STANDARD 99)

add_executable(Lab8 main.c)

target_link_libraries(Lab8 pthread)
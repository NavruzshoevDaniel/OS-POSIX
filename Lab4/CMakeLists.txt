cmake_minimum_required(VERSION 3.17)
project(Lab4 C)

set(CMAKE_C_STANDARD 99)

add_executable(Lab4 main.c)

target_link_libraries(Lab4 pthread)
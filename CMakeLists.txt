cmake_minimum_required(VERSION 3.7)
project(ctest)

set(CMAKE_C_STANDARD 99)

file(GLOB SRC *.c .h)

add_executable(ctest ${SRC})


include_directories(/usr/include)
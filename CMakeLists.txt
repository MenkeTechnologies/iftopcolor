cmake_minimum_required(VERSION 3.7)
project(ctest)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES iftop.c)
add_executable(ctest ${SOURCE_FILES})


include_directories(/usr/include)
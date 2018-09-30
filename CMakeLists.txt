cmake_minimum_required(VERSION 3.12)
project(iftopcolor)

file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/*.c)
add_executable(iftopcolor ${SRC_FILES})
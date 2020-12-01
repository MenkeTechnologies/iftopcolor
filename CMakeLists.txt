cmake_minimum_required(VERSION 3.12)
project(iftopcolor)

if (APPLE)
    include_directories(/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX11.0.sdk/usr/include )
endif (APPLE)

include_directories(${PROJECT_SOURCE_DIR})

file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/*.c)
add_executable(iftopcolor ${SRC_FILES})
cmake_minimum_required(VERSION 3.12)
project(iftop)

if (APPLE)
    include_directories(/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include )
endif (APPLE)

include_directories(${PROJECT_SOURCE_DIR/src})

set(CL "-lpcap -lncurses -lm -lpthread")

file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.c )
add_executable(iftop ${SRC_FILES})

target_link_libraries(iftop ${CL})



cmake_minimum_required(VERSION 3.10)
project(iftop)

if (APPLE)
    include_directories(/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include )
    set(CL "-lpcap -lncurses -lm -lpthread")
else()
    set(CL "-lpcap -lncursesw -lm -lpthread")
endif (APPLE)

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto -funroll-loops -fomit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto")

include_directories(${PROJECT_SOURCE_DIR}/src/include)

file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*/*.c ${PROJECT_SOURCE_DIR}/src/data/*/*.c )

add_executable(iftop ${SRC_FILES})

target_link_libraries(iftop ${CL})

install(TARGETS iftop DESTINATION sbin)



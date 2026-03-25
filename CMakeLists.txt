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

# --- Cyberpunk build banners ---
add_custom_target(build-banner
    COMMAND ${PROJECT_SOURCE_DIR}/build_banner.sh build-start)
add_dependencies(iftop build-banner)
add_custom_command(TARGET iftop POST_BUILD
    COMMAND ${PROJECT_SOURCE_DIR}/build_banner.sh build-end)

add_custom_target(cyberpunk-clean
    COMMAND ${PROJECT_SOURCE_DIR}/build_banner.sh clean-start
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${PROJECT_SOURCE_DIR}/build_banner.sh clean-end
)

# --- Benchmarks ---
# Shared sources needed by benchmarks (no UI, no pcap, no resolver)
set(BENCH_COMMON_SRCS
    ${PROJECT_SOURCE_DIR}/src/data/map/addr_hash.c
    ${PROJECT_SOURCE_DIR}/src/data/map/hash.c
    ${PROJECT_SOURCE_DIR}/src/data/map/stringmap.c
    ${PROJECT_SOURCE_DIR}/src/data/sorted_list.c
    ${PROJECT_SOURCE_DIR}/src/data/vector.c
    ${PROJECT_SOURCE_DIR}/src/util/util.c
)

set(BENCH_NAMES hash sorted_list vector pool stringmap)

foreach(b IN LISTS BENCH_NAMES)
    add_executable(benchmark_${b} EXCLUDE_FROM_ALL bench/bench_${b}.c ${BENCH_COMMON_SRCS})
    target_compile_options(benchmark_${b} PRIVATE -O3 -DNDEBUG -march=native -flto -funroll-loops -fomit-frame-pointer)
    set_target_properties(benchmark_${b} PROPERTIES LINK_FLAGS "-flto")
    target_link_libraries(benchmark_${b} -lm)
endforeach()

# Benchmarks that need additional sources beyond BENCH_COMMON_SRCS
add_executable(benchmark_ns_hash EXCLUDE_FROM_ALL bench/bench_ns_hash.c ${BENCH_COMMON_SRCS}
    ${PROJECT_SOURCE_DIR}/src/data/map/ns_hash.c)
target_compile_options(benchmark_ns_hash PRIVATE -O3 -DNDEBUG -march=native -flto -funroll-loops -fomit-frame-pointer)
set_target_properties(benchmark_ns_hash PROPERTIES LINK_FLAGS "-flto")
target_link_libraries(benchmark_ns_hash -lm)

add_executable(benchmark_serv_hash EXCLUDE_FROM_ALL bench/bench_serv_hash.c ${BENCH_COMMON_SRCS}
    ${PROJECT_SOURCE_DIR}/src/data/map/serv_hash.c)
target_compile_options(benchmark_serv_hash PRIVATE -O3 -DNDEBUG -march=native -flto -funroll-loops -fomit-frame-pointer)
set_target_properties(benchmark_serv_hash PROPERTIES LINK_FLAGS "-flto")
target_link_libraries(benchmark_serv_hash -lm)

add_executable(benchmark_cfgfile EXCLUDE_FROM_ALL bench/bench_cfgfile.c ${BENCH_COMMON_SRCS}
    ${PROJECT_SOURCE_DIR}/src/config/cfgfile.c)
target_compile_options(benchmark_cfgfile PRIVATE -O3 -DNDEBUG -march=native -flto -funroll-loops -fomit-frame-pointer)
set_target_properties(benchmark_cfgfile PROPERTIES LINK_FLAGS "-flto")
target_link_libraries(benchmark_cfgfile -lm)

# "make bench" builds all benchmarks, then runs them
add_custom_target(bench
    DEPENDS benchmark_hash benchmark_sorted_list benchmark_vector benchmark_pool benchmark_stringmap
            benchmark_ns_hash benchmark_serv_hash benchmark_cfgfile
    COMMAND ${PROJECT_SOURCE_DIR}/bench/banner.sh start
    COMMAND $<TARGET_FILE:benchmark_hash>
    COMMAND $<TARGET_FILE:benchmark_sorted_list>
    COMMAND $<TARGET_FILE:benchmark_vector>
    COMMAND $<TARGET_FILE:benchmark_pool>
    COMMAND $<TARGET_FILE:benchmark_stringmap>
    COMMAND $<TARGET_FILE:benchmark_ns_hash>
    COMMAND $<TARGET_FILE:benchmark_serv_hash>
    COMMAND sh -c "$<TARGET_FILE:benchmark_cfgfile> 2>/dev/null"
    COMMAND ${PROJECT_SOURCE_DIR}/bench/banner.sh end
)

# --- Tests ---
set(TEST_COMMON_SRCS
    ${PROJECT_SOURCE_DIR}/src/data/map/hash.c
    ${PROJECT_SOURCE_DIR}/src/data/map/addr_hash.c
    ${PROJECT_SOURCE_DIR}/src/data/map/ns_hash.c
    ${PROJECT_SOURCE_DIR}/src/data/map/serv_hash.c
    ${PROJECT_SOURCE_DIR}/src/data/map/stringmap.c
    ${PROJECT_SOURCE_DIR}/src/data/sorted_list.c
    ${PROJECT_SOURCE_DIR}/src/data/vector.c
    ${PROJECT_SOURCE_DIR}/src/config/cfgfile.c
    ${PROJECT_SOURCE_DIR}/src/util/util.c
)

set(TEST_NAMES vector hash addr_hash sorted_list stringmap ns_hash serv_hash cfgfile util)

foreach(t IN LISTS TEST_NAMES)
    add_executable(check_${t} EXCLUDE_FROM_ALL test/test_${t}.c ${TEST_COMMON_SRCS})
    target_compile_options(check_${t} PRIVATE -Wall -Wextra -Wno-unused-parameter)
endforeach()

# "make check" builds and runs all tests
add_custom_target(check
    DEPENDS check_vector check_hash check_addr_hash check_sorted_list
            check_stringmap check_ns_hash check_serv_hash check_cfgfile check_util
    COMMAND ${PROJECT_SOURCE_DIR}/test/banner.sh start
    COMMAND $<TARGET_FILE:check_util>
    COMMAND $<TARGET_FILE:check_vector>
    COMMAND $<TARGET_FILE:check_hash>
    COMMAND $<TARGET_FILE:check_addr_hash>
    COMMAND $<TARGET_FILE:check_ns_hash>
    COMMAND $<TARGET_FILE:check_serv_hash>
    COMMAND $<TARGET_FILE:check_sorted_list>
    COMMAND $<TARGET_FILE:check_stringmap>
    COMMAND $<TARGET_FILE:check_cfgfile>
    COMMAND ${PROJECT_SOURCE_DIR}/test/banner.sh end
)



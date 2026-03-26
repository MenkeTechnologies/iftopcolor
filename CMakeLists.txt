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

install(CODE "execute_process(COMMAND ${PROJECT_SOURCE_DIR}/scripts/build_banner.sh install-start)")
install(TARGETS iftop DESTINATION sbin)
install(FILES ${PROJECT_SOURCE_DIR}/man/iftop.8 DESTINATION share/man/man8)
install(CODE "execute_process(COMMAND ${PROJECT_SOURCE_DIR}/scripts/build_banner.sh install-end)")

# --- Cyberpunk build banners ---
add_custom_target(build-banner
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/build_banner.sh build-start)
add_dependencies(iftop build-banner)
add_custom_command(TARGET iftop POST_BUILD
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/build_banner.sh build-end)

add_custom_target(cyberpunk-clean
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/build_banner.sh clean-start
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/build_banner.sh clean-end
)

add_custom_target(distclean
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/build_banner.sh distclean-start
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf
        ${CMAKE_BINARY_DIR}/build
        ${CMAKE_BINARY_DIR}/CMakeCache.txt
        ${CMAKE_BINARY_DIR}/CMakeFiles
        ${CMAKE_BINARY_DIR}/cmake_install.cmake
        ${CMAKE_BINARY_DIR}/install_manifest.txt
        ${CMAKE_BINARY_DIR}/Makefile
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_BINARY_DIR}/CTestTestfile.cmake
        ${CMAKE_BINARY_DIR}/Testing
        ${CMAKE_BINARY_DIR}/bench/CMakeFiles
        ${CMAKE_BINARY_DIR}/tags
        ${CMAKE_BINARY_DIR}/GPATH
        ${CMAKE_BINARY_DIR}/GRTAGS
        ${CMAKE_BINARY_DIR}/GTAGS
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/build_banner.sh distclean-end
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

set(BENCH_OPTS -O3 -DNDEBUG -march=native -flto -funroll-loops -fomit-frame-pointer)

function(add_benchmark name)
    set(extra_srcs ${ARGN})
    add_executable(benchmark_${name} EXCLUDE_FROM_ALL bench/bench_${name}.c ${BENCH_COMMON_SRCS} ${extra_srcs})
    target_compile_options(benchmark_${name} PRIVATE ${BENCH_OPTS})
    set_target_properties(benchmark_${name} PROPERTIES LINK_FLAGS "-flto")
    target_link_libraries(benchmark_${name} -lm)
endfunction()

add_benchmark(hash)
add_benchmark(sorted_list)
add_benchmark(vector)
add_benchmark(pool)
add_benchmark(stringmap)
add_benchmark(ns_hash    ${PROJECT_SOURCE_DIR}/src/data/map/ns_hash.c)
add_benchmark(serv_hash  ${PROJECT_SOURCE_DIR}/src/data/map/serv_hash.c)
add_benchmark(cfgfile    ${PROJECT_SOURCE_DIR}/src/config/cfgfile.c)

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
    COMMAND ${PROJECT_SOURCE_DIR}/bench/run_quiet.sh $<TARGET_FILE:benchmark_cfgfile>
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

set(TEST_NAMES vector hash addr_hash sorted_list stringmap ns_hash serv_hash cfgfile util leaks integration options)

foreach(t IN LISTS TEST_NAMES)
    add_executable(check_${t} EXCLUDE_FROM_ALL test/test_${t}.c ${TEST_COMMON_SRCS})
    target_compile_options(check_${t} PRIVATE -Wall -Wextra -Wno-unused-parameter)
endforeach()

target_sources(check_options PRIVATE ${PROJECT_SOURCE_DIR}/src/config/options.c)

# "make check" builds and runs all tests
add_custom_target(check
    DEPENDS check_vector check_hash check_addr_hash check_sorted_list
            check_stringmap check_ns_hash check_serv_hash check_cfgfile check_util
            check_integration check_options
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
    COMMAND $<TARGET_FILE:check_options>
    COMMAND $<TARGET_FILE:check_integration>
    COMMAND ${PROJECT_SOURCE_DIR}/test/banner.sh end
)

# "make leakcheck" runs leak tests under macOS `leaks` tool
add_custom_target(leakcheck
    DEPENDS check_leaks
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  Running memory leak analysis..."
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND leaks --atExit -q -- $<TARGET_FILE:check_leaks>
)



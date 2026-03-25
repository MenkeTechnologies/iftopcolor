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

set(BENCH_TARGETS bench_hash bench_sorted_list bench_vector bench_pool bench_stringmap)

foreach(bench IN LISTS BENCH_TARGETS)
    add_executable(${bench} EXCLUDE_FROM_ALL bench/${bench}.c ${BENCH_COMMON_SRCS})
    target_compile_options(${bench} PRIVATE -O3 -DNDEBUG -march=native -flto -funroll-loops -fomit-frame-pointer)
    set_target_properties(${bench} PROPERTIES LINK_FLAGS "-flto")
    target_link_libraries(${bench} -lm)
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES $<TARGET_FILE:${bench}>)
endforeach()

# "make bench" builds all benchmarks, then runs them
add_custom_target(bench
    DEPENDS ${BENCH_TARGETS}
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running benchmarks ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND $<TARGET_FILE:bench_hash>
    COMMAND $<TARGET_FILE:bench_sorted_list>
    COMMAND $<TARGET_FILE:bench_vector>
    COMMAND $<TARGET_FILE:bench_pool>
    COMMAND $<TARGET_FILE:bench_stringmap>
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

set(TEST_TARGETS
    test_vector test_hash test_addr_hash test_sorted_list
    test_stringmap test_ns_hash test_serv_hash test_cfgfile test_util
)

foreach(t IN LISTS TEST_TARGETS)
    add_executable(${t} EXCLUDE_FROM_ALL test/${t}.c ${TEST_COMMON_SRCS})
    target_compile_options(${t} PRIVATE -Wall -Wextra -Wno-unused-parameter)
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES $<TARGET_FILE:${t}>)
endforeach()

# "make test" builds and runs all tests
add_custom_target(test
    DEPENDS ${TEST_TARGETS}
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running tests ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND $<TARGET_FILE:test_util>
    COMMAND $<TARGET_FILE:test_vector>
    COMMAND $<TARGET_FILE:test_hash>
    COMMAND $<TARGET_FILE:test_addr_hash>
    COMMAND $<TARGET_FILE:test_ns_hash>
    COMMAND $<TARGET_FILE:test_serv_hash>
    COMMAND $<TARGET_FILE:test_sorted_list>
    COMMAND $<TARGET_FILE:test_stringmap>
    COMMAND $<TARGET_FILE:test_cfgfile>
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== All tests passed ==="
)



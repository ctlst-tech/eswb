cmake_minimum_required(VERSION 3.6)
project(eswb)
set(CMAKE_CXX_STANDARD 14)

if (NOT FAKE_PTHREAD)
    if (UNIX AND NOT APPLE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -D_GNU_SOURCE")
    endif()
endif()
#set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(Catch2 REQUIRED)

add_subdirectory(src/lib)
add_subdirectory(src/lib/platformic/posix)

set(TEST_SRC_COMMON
        tests/tooling.cpp
        tests/tooling.h
        tests/sdtl_tooling.cpp
        tests/sdtl_tooling.h
        )

add_executable(eswb_test_basic tests/tests.cpp tests/tests.h tests/event_chain.c ${TEST_SRC_COMMON})
add_executable(eswb_test_eqrb tests/eqrb.cpp  ${TEST_SRC_COMMON})
add_executable(eswb_test_sdtl tests/sdtl.cpp ${TEST_SRC_COMMON})


add_executable(eswb_test_all
        tests/tests.cpp
        tests/tests.h
        tests/event_chain.c
        tests/sdtl.cpp
        tests/eqrb.cpp
        tests/bbee_framing.cpp
        ${TEST_SRC_COMMON})

target_include_directories(eswb_test_basic PRIVATE src/lib/include)
target_link_libraries(eswb_test_basic PUBLIC eswb-static eswb-sync-static Catch2::Catch2WithMain)

target_include_directories(eswb_test_eqrb PRIVATE src/lib/include)
target_link_libraries(eswb_test_eqrb PUBLIC eswb-static eswb-eqrb-static eswb-sdtl-static eswb-sync-static Catch2::Catch2WithMain)

target_include_directories(eswb_test_sdtl PRIVATE src/lib/include)
target_link_libraries(eswb_test_sdtl PUBLIC eswb-static eswb-sdtl-static eswb-sync-static Catch2::Catch2WithMain)

target_include_directories(eswb_test_all PRIVATE src/lib/include)
target_link_libraries(eswb_test_all PUBLIC eswb-static eswb-eqrb-static eswb-sdtl-static eswb-sync-static Catch2::Catch2WithMain)

add_executable(eswb_test_dummy tests/eswb_test_dummy.c tests/event_chain.c)
target_link_libraries(eswb_test_dummy PUBLIC m)
target_link_libraries(eswb_test_dummy PUBLIC eswb)


add_library(eswb-if INTERFACE)
target_link_libraries(eswb-if INTERFACE eswb-static eswb-sync-static eswb-eqrb-static)

add_library(eswb-cpp STATIC cpp/eswb.hpp cpp/eswb.cpp)
target_link_libraries(eswb-cpp PUBLIC eswb-static eswb-sync-static eswb-eqrb-static)

add_executable(eswbutil cpp/main.cpp)
target_link_libraries(eswbutil eswb-cpp eswb-static eswb-sync-static eswb-eqrb-static)

install(TARGETS eswbutil RUNTIME)
install(TARGETS eswb_test_dummy RUNTIME)
install(TARGETS eswb-cpp LIBRARY)

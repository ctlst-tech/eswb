
#find_package(Catch2 REQUIRED)
#
#add_subdirectory(../src/lib EXCLUDE_FROM_ALL)
#add_subdirectory(../src/lib/platformic/posix EXCLUDE_FROM_ALL)
#
#add_executable(eswb_c_test tests/event_chain.c)
#
#add_executable(eswb_catch2_test tests/test-main.cpp tests/tests.cpp tests/tests.h tests/event_chain.c)
#
#target_link_libraries(eswb_c_test PUBLIC m)
#target_link_libraries(eswb_c_test PUBLIC eswb-static eswb-sync-static)
#target_compile_options(eswb_c_test PUBLIC -DESWB_C_TEST=1)
#
#target_link_libraries(eswb_catch2_test PUBLIC eswb-static eswb-sync-static Catch2::Catch2)
##target_compile_options(eswb_catch2_test PUBLIC -DESWB_UNIT_TESTS=1)
#target_include_directories(eswb_catch2_test PRIVATE src/include)

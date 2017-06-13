cmake_minimum_required(VERSION 3.2)
project (any_with_traits_test)

set (gtest_files
   ${PROJECT_SOURCE_DIR}/gtest.h
   ${PROJECT_SOURCE_DIR}/gtest-all.cc
   ${PROJECT_SOURCE_DIR}/test.cpp
   ${PROJECT_SOURCE_DIR}/main.cpp
)

include_directories (../src)

set (CMAKE_CXX_STANDARD 14)
find_package (Threads)
add_executable (any_with_traits_test ${gtest_files})
target_link_libraries (any_with_traits_test ${CMAKE_THREAD_LIBS_INIT})

set (GENERIC_OUTPUT_DIR, ${BIN_DIR})

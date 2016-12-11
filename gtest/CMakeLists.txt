cmake_minimum_required(VERSION 3.5)
project (any_with_traits_test)

set (gtest_files
   ${PROJECT_SOURCE_DIR}/gtest.h
   ${PROJECT_SOURCE_DIR}/gtest-all.cc
   ${PROJECT_SOURCE_DIR}/test.cpp
   ${PROJECT_SOURCE_DIR}/main.cpp
)

include_directories (../src)

add_executable (any_with_traits_test ${gtest_files})

set (GENERIC_OUTPUT_DIR, ${BIN_DIR})
add_definitions ("-D_UNICODE -DUNICODE -D_WIN32_WINNT=0x501 -DWINVER=0x501")

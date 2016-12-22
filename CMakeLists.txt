cmake_minimum_required(VERSION 3.5)
add_subdirectory ("gtest")
add_subdirectory ("src")

project (any_with_traits)

set (GENERIC_OUTPUT_DIR, ${BIN_DIR})
add_definitions ("-D_UNICODE -DUNICODE -D_WIN32_WINNT=0x501 -DWINVER=0x501")



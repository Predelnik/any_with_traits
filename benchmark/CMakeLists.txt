cmake_minimum_required(VERSION 3.5)
project (any_with_traits_benchmark)

set (gtest_files
   ${PROJECT_SOURCE_DIR}/nonius.h++
   ${PROJECT_SOURCE_DIR}/benchmark.cpp
)

include_directories (../src)

set (CMAKE_CXX_STANDARD 14)
find_package (Threads)
find_package ( Boost 1.60)
if (${Boost_FOUND})
	include_directories ( ${Boost_INCLUDE_DIR} )
	add_executable (any_with_traits_benchmark ${gtest_files})
	target_link_libraries (any_with_traits_benchmark ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
else ()
	message ("Boost not found. Benchmark disabled. Define BOOST_ROOT if you have it.")
endif ()

set (GENERIC_OUTPUT_DIR, ${BIN_DIR})
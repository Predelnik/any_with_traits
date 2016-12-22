cmake_minimum_required(VERSION 3.5)
project (any_with_traits CXX)

set (src_files
   ${PROJECT_SOURCE_DIR}/any_with_traits.h
   ${PROJECT_SOURCE_DIR}/overload.h
)

add_library (any_with_traits ${src_files})
set_target_properties(any_with_traits PROPERTIES LINKER_LANGUAGE CXX)


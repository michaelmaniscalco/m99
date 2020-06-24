cmake_minimum_required(VERSION 3.16.3)

project(m99)

include(ExternalProject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)

add_compile_options(
    -O3
)

find_package(Threads REQUIRED)

find_library(LIBCXX_LIB c++)
find_library(LIBCXXABI_LIB c++abi)

set(CMAKE_CXX_COMPILER "clang++") 
add_compile_options(-stdlib=libstdc++)
link_libraries(
    ${LIBCXX_LIB}
    ${LIBCXXABI_LIB}
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)


cmake_minimum_required(VERSION 3.16.3)

project(m99)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message("*** Build type not set.  defaulting to Release")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(
        -g
        -O0
        -march=native
    )
else()
    add_compile_options(
        -O3
        -march=native
    )
endif()


option(M99_BUILD_DEMO "Build the CLI demo" ON)


include(FetchContent)

set(IO_BUILD_DEMO OFF CACHE INTERNAL "")
FetchContent_Declare(
    io
    GIT_REPOSITORY https://github.com/michaelmaniscalco/io.git
    GIT_TAG master
    SOURCE_DIR        "${CMAKE_BINARY_DIR}/io-src"
    BINARY_DIR        "${CMAKE_BINARY_DIR}/io-build"
    INSTALL_DIR       "${CMAKE_BINARY_DIR}"
    INSTALL_COMMAND   ""
)
FetchContent_MakeAvailable(io)
FetchContent_GetProperties(io)


set(ENTROPY_BUILD_DEMO OFF CACHE INTERNAL "")
FetchContent_Declare(
    entropy
    GIT_REPOSITORY https://github.com/michaelmaniscalco/entropy.git
    GIT_TAG master
    SOURCE_DIR        "${CMAKE_BINARY_DIR}/entropy-src"
    BINARY_DIR        "${CMAKE_BINARY_DIR}/entropy-build"
    INSTALL_DIR       "${CMAKE_BINARY_DIR}"
    INSTALL_COMMAND   ""
)
FetchContent_MakeAvailable(entropy)
FetchContent_GetProperties(entropy)



add_subdirectory(src)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

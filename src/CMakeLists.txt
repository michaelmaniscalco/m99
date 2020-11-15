set(_m99_include_dir ${CMAKE_CURRENT_SOURCE_DIR})

# fetch library dependencies in other repos
include(FetchContent)

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



add_subdirectory(library)
add_subdirectory(executable)
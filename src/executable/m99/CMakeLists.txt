
include(FetchContent)

FetchContent_Declare(
    msufsort
    GIT_REPOSITORY https://github.com/michaelmaniscalco/msufsort.git
    GIT_TAG master
    SOURCE_DIR        "${CMAKE_BINARY_DIR}/msufsort-src"
    BINARY_DIR        "${CMAKE_BINARY_DIR}/msufsort-build"
    INSTALL_DIR       "${CMAKE_BINARY_DIR}"
    INSTALL_COMMAND   ""
)
FetchContent_MakeAvailable(msufsort)
FetchContent_GetProperties(msufsort)

find_library(LIBCXX_LIB c++)
find_package(Threads)
find_library(LIBCXXABI_LIB c++abi)

add_compile_options(
    -O3
    -march=native
)
link_libraries(
    ${LIBCXX_LIB}
    ${LIBCXXABI_LIB}
)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(m99_demo main.cpp)

target_link_libraries(m99_demo ${CMAKE_THREAD_LIBS_INIT} m99 msufsort)
add_library(m99
    m99_decode.cpp
    m99_encode.cpp
    m99_encode_stream.cpp
    m99_decode_stream.cpp
)

target_link_libraries(m99 io entropy)

target_include_directories(m99
    PUBLIC
        $<BUILD_INTERFACE:${_m99_include_dir}>
        $<INSTALL_INTERFACE:include/m99>
)

target_compile_features(m99 PUBLIC cxx_std_17)

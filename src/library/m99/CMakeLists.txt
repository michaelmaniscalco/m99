add_library(m99
    input_buffer.cpp
    input_stream.cpp
    output_buffer.cpp
    output_stream.cpp
    m99.cpp
)

target_include_directories(m99
    PUBLIC
        $<BUILD_INTERFACE:${_m99_include_dir}>
        $<INSTALL_INTERFACE:include/m99>
)

target_compile_features(m99 PUBLIC cxx_std_17)

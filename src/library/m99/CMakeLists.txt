add_library(m99_lib
    input_buffer.cpp
    input_stream.cpp
    output_buffer.cpp
    output_stream.cpp
    m99.cpp
)

target_link_libraries(m99_lib
    PUBLIC
        msufsort
)

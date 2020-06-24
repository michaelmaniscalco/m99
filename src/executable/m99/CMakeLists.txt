add_executable(m99 main.cpp)

target_link_libraries(m99 
PRIVATE
    Threads::Threads
    m99_lib
)
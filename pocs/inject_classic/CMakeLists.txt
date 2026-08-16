cmake_minimum_required(VERSION 3.16)

add_executable(inject_classic main.c)
target_link_libraries(inject_classic PRIVATE sindri::engine)
set_property(TARGET inject_classic PROPERTY FOLDER "PoCs")

add_executable(loader_coff main.c)
target_link_libraries(loader_coff PRIVATE sindri::engine)

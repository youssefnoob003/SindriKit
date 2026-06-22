add_executable(loader_winapi main.c)
target_link_libraries(loader_winapi PRIVATE sindri::engine)

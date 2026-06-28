add_executable(inject_shell main.c)
target_link_libraries(inject_shell PRIVATE sindri::engine)

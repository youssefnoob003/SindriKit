add_executable(inject_pe main.c)
target_link_libraries(inject_pe PRIVATE sindri::engine)

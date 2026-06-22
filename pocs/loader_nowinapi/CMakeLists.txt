add_executable(loader_nowinapi main.c)
target_link_libraries(loader_nowinapi PRIVATE sindri::engine)

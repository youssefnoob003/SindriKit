add_executable(loader_noCRT_nowinapi main.c)
target_link_libraries(loader_noCRT_nowinapi PRIVATE sindri::engine)

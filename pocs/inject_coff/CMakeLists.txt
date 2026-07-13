project(inject_coff C)

add_executable(inject_coff main.c)

target_link_libraries(inject_coff PRIVATE 
    sindri_engine
)

target_compile_definitions(inject_coff PRIVATE SND_USE_DEFAULTS)

set_target_properties(inject_coff PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pocs/inject_coff"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/pocs/inject_coff/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/pocs/inject_coff/Release"
)

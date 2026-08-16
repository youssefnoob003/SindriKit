cmake_minimum_required(VERSION 3.16)

# Provide a default target image path (Notepad is a good candidate)
set(SND_DEFAULT_APC_TARGET "C:\\\\Windows\\\\System32\\\\notepad.exe")

add_executable(inject_apc main.c)
target_link_libraries(inject_apc PRIVATE sindri::engine)

target_compile_definitions(inject_apc PRIVATE
    SND_DEFAULT_APC_TARGET="${SND_DEFAULT_APC_TARGET}"
)

set_property(TARGET inject_apc PROPERTY FOLDER "PoCs")

add_executable(test_exe src/test_exe.c)
add_library(test_dll SHARED src/test_dll.c)

add_executable(test_exe_advanced src/test_exe_advanced.c)
add_library(test_dll_advanced SHARED src/test_dll_advanced.c)

add_library(test_dll_empty SHARED src/test_dll_empty.c)
add_library(test_dll_tls SHARED src/test_dll_tls.c)

# Create an architecture suffix (e.g., _x64 or _x86)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_SUFFIX "_x64")
else()
    set(ARCH_SUFFIX "_x86")
endif()

# Apply suffixes to the output names of the tests so they match what test_runner.py expects
set_target_properties(test_exe PROPERTIES OUTPUT_NAME "test_exe${ARCH_SUFFIX}")
set_target_properties(test_dll PROPERTIES OUTPUT_NAME "test_dll${ARCH_SUFFIX}")

set_target_properties(test_exe_advanced PROPERTIES OUTPUT_NAME "test_exe_advanced${ARCH_SUFFIX}")
set_target_properties(test_dll_advanced PROPERTIES OUTPUT_NAME "test_dll_advanced${ARCH_SUFFIX}")

set_target_properties(test_dll_empty PROPERTIES OUTPUT_NAME "test_dll_empty${ARCH_SUFFIX}")

set_target_properties(test_dll_tls PROPERTIES OUTPUT_NAME "test_dll_tls${ARCH_SUFFIX}")

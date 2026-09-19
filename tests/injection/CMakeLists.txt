# Injection test fixtures.
#
#   test_inject_target        - lightweight console target the techniques
#                               inject into (spawned by the runner for classic,
#                               by the engine for APC/hijack).
#   test_inject_marker DLL    - PE payload whose entry point writes the marker.
#   test_inject_marker BOF    - COFF payload whose `go` writes the marker.
#
# The runner waits for the marker file (written into the target's directory) as
# the injected side effect.

add_executable(test_inject_target src/test_inject_target.c)
add_library(test_inject_marker_dll SHARED src/test_inject_marker_dll.c)
add_library(test_inject_marker_bof OBJECT src/test_inject_marker_bof.c)

# Payloads must not reference CRT functions:
#  - the PE payload is CRT-less (/NODEFAULTLIB) because injection starts a
#    thread at the entry point with no arguments, so DllMainCRTStartup would
#    fail with STATUS_DLL_INIT_FAILED;
#  - the COFF payload is linked by the COFF loader, which only resolves
#    MODULE$Func imports.
# /Od prevents loop-idiom recognition from synthesizing memcpy/memset, and
# /Gs- style stack-probe suppression avoids __chkstk. /ENTRY is used as-is on
# both architectures (the x86 linker applies the leading underscore itself).
target_compile_options(test_inject_marker_dll PRIVATE /GS- /Od /Gs2147483647)
target_link_options(test_inject_marker_dll PRIVATE "/ENTRY:snd_marker_entry" /NODEFAULTLIB)

target_compile_options(test_inject_marker_bof PRIVATE /GS- /Zc:threadSafeInit- /Gy /Od)

# The target is CRT-less too: it depends only on kernel32, so it starts even if
# the engine's hand-built process-parameter block omits optional fields (DllPath
# etc.). This keeps the test focused on the injection mechanism.
target_compile_options(test_inject_target PRIVATE /GS- /Od /Gs2147483647)
target_link_options(test_inject_target PRIVATE "/ENTRY:snd_target_entry" /NODEFAULTLIB)
target_link_libraries(test_inject_target PRIVATE kernel32)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_SUFFIX "_x64")
else()
    set(ARCH_SUFFIX "_x86")
endif()

set_target_properties(test_inject_target PROPERTIES OUTPUT_NAME "test_inject_target${ARCH_SUFFIX}")
set_target_properties(test_inject_marker_dll PROPERTIES OUTPUT_NAME "test_inject_marker${ARCH_SUFFIX}")

add_custom_target(copy_inject_bof ALL
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_OBJECTS:test_inject_marker_bof>
        ${CMAKE_CURRENT_BINARY_DIR}/test_inject_marker${ARCH_SUFFIX}.obj
    DEPENDS test_inject_marker_bof
)
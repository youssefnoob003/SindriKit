add_library(test_coff_basic OBJECT src/test_coff_basic.c)
add_library(test_coff_reloc OBJECT src/test_coff_reloc.c)
add_library(test_coff_args OBJECT src/test_coff_args.c)
add_library(test_coff_msgbox OBJECT src/test_coff_msgbox.c)

target_compile_options(test_coff_basic PRIVATE /GS- /Zc:threadSafeInit- /Gy /Os)
target_compile_options(test_coff_reloc PRIVATE /GS- /Zc:threadSafeInit- /Gy /Os)
target_compile_options(test_coff_args PRIVATE /GS- /Zc:threadSafeInit- /Gy /Os)
target_compile_options(test_coff_msgbox PRIVATE /GS- /Zc:threadSafeInit- /Gy /Os)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_SUFFIX "_x64")
else()
    set(ARCH_SUFFIX "_x86")
endif()

add_custom_target(copy_coff_tests ALL
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_OBJECTS:test_coff_basic>
    ${CMAKE_CURRENT_BINARY_DIR}/test_coff_basic${ARCH_SUFFIX}.obj
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_OBJECTS:test_coff_reloc>
    ${CMAKE_CURRENT_BINARY_DIR}/test_coff_reloc${ARCH_SUFFIX}.obj
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_OBJECTS:test_coff_args>
    ${CMAKE_CURRENT_BINARY_DIR}/test_coff_args${ARCH_SUFFIX}.obj
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_OBJECTS:test_coff_msgbox>
    ${CMAKE_CURRENT_BINARY_DIR}/test_coff_msgbox${ARCH_SUFFIX}.obj
    DEPENDS test_coff_basic test_coff_reloc test_coff_args test_coff_msgbox
)

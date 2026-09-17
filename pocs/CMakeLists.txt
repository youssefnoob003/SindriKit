set(UNIFIED_SOURCES
    src/main.c
    src/print.c
    src/cli.c
    src/runtime.c
    src/backend.c
    src/cmd_load_pe.c
    src/cmd_load_coff.c
    src/cmd_inject_classic.c
    src/cmd_inject_apc.c
    src/cmd_hg.c
)

add_executable(unified
    ${UNIFIED_SOURCES}
)

if(SND_CRTLESS)
    target_sources(unified PRIVATE src/frontend_crtless.c)
endif()

target_include_directories(unified PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(unified PRIVATE sindri::engine)

if(MSVC)
    target_compile_options(unified PRIVATE /W4 /WX-)
endif()

if(SND_CRTLESS)
    target_compile_definitions(unified PRIVATE SND_CRTLESS=1)

    if(MSVC)
        target_compile_options(unified PRIVATE /GS- /sdl- /Gs2147483647)
        target_link_options(unified PRIVATE
            /NODEFAULTLIB
            /ENTRY:snd_crtless_poc_entry
        )
    endif()
else()
    target_compile_definitions(unified PRIVATE SND_USE_WINDOWS_SDK=1)
endif()

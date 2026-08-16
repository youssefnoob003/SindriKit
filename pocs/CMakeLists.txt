if(SND_CRTLESS)
    add_subdirectory(loader_noCRT_nowinapi)
    target_link_options(loader_noCRT_nowinapi PRIVATE
        /NODEFAULTLIB
        /ENTRY:main
    )
else()
    add_subdirectory(loader_winapi)
    add_subdirectory(loader_nowinapi)
    add_subdirectory(loader_coff)
    add_subdirectory(heavens_gate)
    add_subdirectory(inject_classic)
    add_subdirectory(inject_apc)
endif()

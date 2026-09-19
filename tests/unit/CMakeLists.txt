add_executable(snd_unit_tests
    test_framework.c
    pe_builder.c
    coff_builder.c
    test_main.c
    test_guards.c
    test_hijack.c
    test_pe_parser.c
    test_pe_exports.c
    test_pe_imports.c
    test_pe_relocs.c
    test_coff_parser.c
    test_coff_symbols.c
    test_inject_chains.c
)

target_link_libraries(snd_unit_tests PRIVATE sindri::engine)

if(MSVC)
    target_compile_options(snd_unit_tests PRIVATE /W4 /WX-)
endif()

# One ctest entry per framework group so a fault in a single group does not
# hide the rest, and the full suite as a fast aggregate. Groups map to the
# test_framework `--filter` prefixes registered by each test_*.c.
add_test(NAME snd_unit_tests COMMAND snd_unit_tests)

set(SND_UNIT_GROUPS
    common
    guards
    hijack
    "pe parser"
    "pe exports"
    "pe imports"
    "pe relocs"
    "coff parser"
    "coff symbols"
    "inject chains"
)
foreach(group IN LISTS SND_UNIT_GROUPS)
    string(REPLACE " " "_" group_id "${group}")
    add_test(NAME "snd_unit_tests_${group_id}"
             COMMAND snd_unit_tests --filter "${group}")
endforeach()

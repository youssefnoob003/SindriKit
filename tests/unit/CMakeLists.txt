add_executable(snd_unit_tests
    test_main.c
    test_guards.c
)

target_link_libraries(snd_unit_tests PRIVATE sindri::engine)

if(MSVC)
    target_compile_options(snd_unit_tests PRIVATE /W4 /WX-)
endif()

add_test(NAME snd_unit_tests COMMAND snd_unit_tests)

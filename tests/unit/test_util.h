#ifndef SND_TEST_UTIL_H
#define SND_TEST_UTIL_H

/*
 * Shared test helpers. The unit test binary compiles against the engine with
 * SND_DEBUG defined (build.bat tests), so status values carry context.
 */

#include "test_framework.h"

#include <sindri.h>
#include <stdio.h>

/* Group registration entry points. Each test_*.c exports the symbols it was
 * compiled for; all are declared here so test_main.c can invoke them. */
void snd_test_register_common(void);
void snd_test_register_guards(void);
void snd_test_register_hijack(void);
void snd_test_register_pe_parser(void);
void snd_test_register_pe_exports(void);
void snd_test_register_pe_imports(void);
void snd_test_register_pe_relocs(void);
void snd_test_register_coff_parser(void);
void snd_test_register_coff_symbols(void);
void snd_test_register_inject_chains(void);

#endif /* SND_TEST_UTIL_H */
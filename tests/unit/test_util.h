#ifndef SND_TEST_UTIL_H
#define SND_TEST_UTIL_H

#include <stdio.h>

extern int g_test_failures;

#define CHECK(cond)                                                                                                    \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                            \
            g_test_failures++;                                                                                         \
        }                                                                                                              \
    } while (0)

/* Flushed to stderr before each test so a fault can be localized. */
#define PROGRESS(name)                                                                                                 \
    do {                                                                                                               \
        fprintf(stderr, "[unit] %s\n", (name));                                                                        \
        fflush(stderr);                                                                                                \
    } while (0)

void snd_run_guard_tests(void);

#endif // SND_TEST_UTIL_H

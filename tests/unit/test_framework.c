#include "test_framework.h"

#include <stdarg.h>
#include <stdio.h>

static snd_test_case_t g_tests[SND_TEST_MAX_TESTS];
static int             g_count                 = 0;
static const char     *g_current               = NULL;
static int             g_failures_current      = 0;
static int             g_registration_failures = 0;

jmp_buf g_snd_test_jmp_env;

int snd_test_register(const char *name, snd_test_fn_t fn) {
    if (name == NULL || fn == NULL || g_count >= SND_TEST_MAX_TESTS) {
        fprintf(stderr, "unit tests: unable to register test '%s'\n", name != NULL ? name : "<null>");
        g_registration_failures++;
        return -1;
    }
    g_tests[g_count].name = name;
    g_tests[g_count].fn   = fn;
    g_count++;
    return 0;
}

void snd_test_fail(const char *file, int line, const char *expr, const char *detail) {
    fprintf(stderr, "  FAIL [%s] (%s:%d): %s%s%s\n", g_current != NULL ? g_current : "unknown", file, line, expr,
            (detail != NULL) ? ": " : "", (detail != NULL) ? detail : "");
    fflush(stderr);
    g_failures_current++;
    longjmp(g_snd_test_jmp_env, 1);
}

void snd_test_fail_fmt(const char *file, int line, const char *expr, const char *fmt, ...) {
    if (fmt != NULL) {
        char    detail[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(detail, sizeof(detail), fmt, args);
        va_end(args);
        snd_test_fail(file, line, expr, detail);
    } else {
        snd_test_fail(file, line, expr, NULL);
    }
}

int snd_test_main(int argc, char **argv) {
    int         list_only = 0;
    const char *filter    = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--list") == 0) {
            list_only = 1;
        } else if (strcmp(argv[i], "--filter") == 0 && (i + 1 < argc)) {
            filter = argv[++i];
        } else {
            fprintf(stderr, "unit tests: unknown or incomplete argument '%s'\n", argv[i]);
            fprintf(stderr, "usage: snd_unit_tests [--list] [--filter <substring>]\n");
            return 2;
        }
    }

    if (g_registration_failures != 0) {
        fprintf(stderr, "unit tests: %d test registration failure(s)\n", g_registration_failures);
        return 1;
    }

    if (list_only) {
        for (int i = 0; i < g_count; i++) {
            printf("%s\n", g_tests[i].name);
        }
        return 0;
    }

    int ran    = 0;
    int passed = 0;
    int failed = 0;

    for (int i = 0; i < g_count; i++) {
        if (filter != NULL && strstr(g_tests[i].name, filter) == NULL) {
            continue;
        }

        ran++;
        g_current          = g_tests[i].name;
        g_failures_current = 0;

        fprintf(stderr, "[unit] run: %s\n", g_tests[i].name);
        fflush(stderr);

        if (setjmp(g_snd_test_jmp_env) == 0) {
            g_tests[i].fn();
        }

        if (g_failures_current == 0) {
            passed++;
            printf("[unit] OK       %s\n", g_tests[i].name);
        } else {
            failed++;
            printf("[unit] FAILED   %s (%d assertion failure(s))\n", g_tests[i].name, g_failures_current);
        }
    }

    if (ran == 0) {
        printf("unit tests: no tests matched%s%s\n", filter ? " (filter: " : "", filter ? filter : "");
        fflush(stdout);
        return 1;
    }

    printf("unit tests: %d/%d passed, %d failed\n", passed, ran, failed);
    fflush(stdout);
    return failed == 0 ? 0 : 1;
}

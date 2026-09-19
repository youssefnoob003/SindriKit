#ifndef SND_TEST_FRAMEWORK_H
#define SND_TEST_FRAMEWORK_H

#include <setjmp.h>
#include <stddef.h>
#include <string.h>

extern jmp_buf g_snd_test_jmp_env;

#define SND_TEST_MAX_TESTS 512

typedef void (*snd_test_fn_t)(void);

typedef struct snd_test_case {
    const char   *name; /* static string, never freed */
    snd_test_fn_t fn;
} snd_test_case_t;

/*
 * Registers a test case. Returns 0 on success, or -1 when the registry is full.
 * The name must remain valid for the lifetime of the process (use a literal).
 */
int snd_test_register(const char *name, snd_test_fn_t fn);

/*
 * Runs the registered suite.
 *   --list           enumerate all registered test names and exit 0
 *   --filter <substr> run only tests whose name contains the substring
 * Returns a process exit code (0 = all selected tests passed).
 */
int snd_test_main(int argc, char **argv);

/* Records an assertion failure for the currently running test. */
void snd_test_fail(const char *file, int line, const char *expr, const char *detail);

/* Variant of snd_test_fail with a printf-style detail message. */
void snd_test_fail_fmt(const char *file, int line, const char *expr, const char *fmt, ...);

/* ── Assertion macros ──────────────────────────────────────────────────── */

#define SND_CHECK(cond)                                                                                                \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            snd_test_fail(__FILE__, __LINE__, #cond, NULL);                                                            \
        }                                                                                                              \
    } while (0)

#define SND_CHECK_EQ_INT(lhs, rhs)                                                                                     \
    do {                                                                                                               \
        long long _snd_l = (long long)(lhs);                                                                           \
        long long _snd_r = (long long)(rhs);                                                                           \
        if (_snd_l != _snd_r) {                                                                                        \
            snd_test_fail_fmt(__FILE__, __LINE__, #lhs " == " #rhs, "got %lld, expected %lld", _snd_l, _snd_r);        \
        }                                                                                                              \
    } while (0)

#define SND_CHECK_EQ_PTR(lhs, rhs)                                                                                     \
    do {                                                                                                               \
        const void *_snd_l = (const void *)(lhs);                                                                      \
        const void *_snd_r = (const void *)(rhs);                                                                      \
        if (_snd_l != _snd_r) {                                                                                        \
            snd_test_fail(__FILE__, __LINE__, #lhs " == " #rhs, NULL);                                                 \
        }                                                                                                              \
    } while (0)

#define SND_CHECK_EQ_U32(lhs, rhs)                                                                                     \
    do {                                                                                                               \
        unsigned long _snd_l = (unsigned long)(lhs);                                                                   \
        unsigned long _snd_r = (unsigned long)(rhs);                                                                   \
        if (_snd_l != _snd_r) {                                                                                        \
            snd_test_fail_fmt(__FILE__, __LINE__, #lhs " == " #rhs, "got 0x%lX, expected 0x%lX", _snd_l, _snd_r);      \
        }                                                                                                              \
    } while (0)

#define SND_CHECK_STR_EQ(lhs, rhs)                                                                                     \
    do {                                                                                                               \
        const char *_snd_l = (const char *)(lhs);                                                                      \
        const char *_snd_r = (const char *)(rhs);                                                                      \
        if (_snd_l == NULL || _snd_r == NULL || strcmp(_snd_l, _snd_r) != 0) {                                         \
            snd_test_fail(__FILE__, __LINE__, "strcmp(" #lhs ", " #rhs ") == 0", NULL);                                \
        }                                                                                                              \
    } while (0)

/* Engine-status-aware assertion helpers. They expand `snd_status_t` /
 * `SND_SUCCESS` at the use site, so <sindri.h> must be included by the test
 * TU first (test_util.h does this). */
#define SND_STATUS_IS_FAILED(status) ((status).code != SND_SUCCESS)
#define SND_STATUS_IS_OK(status)     ((status).code == SND_SUCCESS)

#define SND_CHECK_SUCCEEDED(status)                                                                                    \
    do {                                                                                                               \
        snd_status_t _snd_st = (status);                                                                               \
        if (SND_STATUS_IS_FAILED(_snd_st)) {                                                                           \
            snd_test_fail_fmt(__FILE__, __LINE__, #status " == SND_OK", "got code 0x%08X", (unsigned)_snd_st.code);    \
        }                                                                                                              \
    } while (0)

#define SND_CHECK_FAILED(status)                                                                                       \
    do {                                                                                                               \
        snd_status_t _snd_st = (status);                                                                               \
        if (SND_STATUS_IS_OK(_snd_st)) {                                                                               \
            snd_test_fail(__FILE__, __LINE__, "SND_FAILED(" #status ")", "status unexpectedly succeeded");             \
        }                                                                                                              \
    } while (0)

#define SND_CHECK_STATUS(status, expected_code)                                                                        \
    do {                                                                                                               \
        snd_status_t _snd_st = (status);                                                                               \
        if (_snd_st.code != (expected_code)) {                                                                         \
            snd_test_fail_fmt(__FILE__, __LINE__, #status " .code == " #expected_code, "got 0x%08X",                   \
                              (unsigned)_snd_st.code);                                                                 \
        }                                                                                                              \
    } while (0)

/* Short aliases used by the test bodies. */
#define CHECK(cond)            SND_CHECK(cond)
#define CHECK_EQ_INT(lhs, rhs) SND_CHECK_EQ_INT(lhs, rhs)
#define CHECK_EQ_U32(lhs, rhs) SND_CHECK_EQ_U32(lhs, rhs)
#define CHECK_EQ_PTR(lhs, rhs) SND_CHECK_EQ_PTR(lhs, rhs)
#define CHECK_STR_EQ(lhs, rhs) SND_CHECK_STR_EQ(lhs, rhs)

#endif /* SND_TEST_FRAMEWORK_H */
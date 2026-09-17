/*
 * Host-side unit tests for the pure, CRT-friendly surface of SindriKit:
 * status encoding, bounds helpers, string helpers, hashing, and the
 * parser rejection paths. Build with SND_BUILD_UNIT_TESTS=ON and run
 * through ctest (Windows only).
 */

#include "test_util.h"

#include <sindri.h>

int        g_test_failures = 0;
static int g_freed         = 0;

static void fake_free(snd_buffer_t *buf) {
    (void)buf;
    g_freed++;
}

static void test_status_encoding(void) {
    int code = SND_MAKE_STATUS(SND_FACILITY_PARSER_PE, 7);
    CHECK(SND_STATUS_FACILITY(code) == SND_FACILITY_PARSER_PE);
    CHECK(SND_STATUS_LOCAL_CODE(code) == 7);

    snd_status_t ok = SND_OK;
    CHECK(SND_SUCCEEDED(ok));
    CHECK(!SND_FAILED(ok));

    snd_status_t err = SND_ERR(SND_STATUS_NULL_POINTER);
    CHECK(SND_FAILED(err));
    CHECK(err.code == SND_STATUS_NULL_POINTER);
}

static void test_memory_bounds(void) {
    CHECK(snd_memory_bounds_check(16, 0, 16) == 1);
    CHECK(snd_memory_bounds_check(16, 16, 1) == 0);
    CHECK(snd_memory_bounds_check(16, 8, 8) == 1);
    CHECK(snd_memory_bounds_check(16, 8, 9) == 0);
    CHECK(snd_memory_bounds_check(0, 0, 1) == 0);

    char  region[16] = {0};
    char *inner      = region + 1;
    CHECK(snd_memory_ptr_bounds_check(region, sizeof(region), region + 8, 8) == 1);
    CHECK(snd_memory_ptr_bounds_check(region, sizeof(region), region + 8, 9) == 0);
    CHECK(snd_memory_ptr_bounds_check(inner, 8, region, 1) == 0);
    CHECK(snd_memory_ptr_bounds_check(NULL, sizeof(region), region, 1) == 0);
}

static void test_range_macros(void) {
    CHECK(SND_RANGE_EXCEEDS(8, 8, 16) == 0);
    CHECK(SND_RANGE_EXCEEDS(8, 9, 16) != 0);
    CHECK(SND_IN_BOUNDS(15, 0, 16) != 0);
    CHECK(SND_IN_BOUNDS(16, 0, 16) == 0);
}

static void test_string_helpers(void) {
    CHECK(snd_strnlen("abc", 8) == 3);
    CHECK(snd_strnlen("abc", 2) == 2);

    char dst[8] = {0};
    snd_strncpy(dst, sizeof(dst), "hello", 5);
    CHECK(dst[5] == '\0');
    CHECK(snd_strncmp(dst, "hello", 8) == 0);

    CHECK(snd_strnchr("abc", 'b', 3) != NULL);
    CHECK(snd_strnchr("abc", 'z', 3) == NULL);
}

static void test_hash_determinism(void) {
    CHECK(snd_hash("kernel32.dll") == snd_hash("kernel32.dll"));
    CHECK(snd_hash("kernel32.dll") != snd_hash("ntdll.dll"));
}

static void test_buffer_lifecycle(void) {
    char         data[16] = {0};
    snd_buffer_t buf      = {0};
    buf.data              = data;
    buf.size              = sizeof(data);

    CHECK(snd_buffer_bounds_check(&buf, 0, 16) == 1);
    CHECK(snd_buffer_bounds_check(&buf, 8, 8) == 1);
    CHECK(snd_buffer_bounds_check(&buf, 8, 9) == 0);

    snd_buffer_t empty = {0};
    CHECK(snd_buffer_bounds_check(&empty, 0, 1) == 0);

    snd_buffer_init(&buf, data, sizeof(data), fake_free);
    snd_buffer_free(&buf);
    CHECK(g_freed == 1);
    CHECK(buf.data == NULL);
    CHECK(buf.free_routine == NULL);
}

static void test_parsers_reject_garbage(void) {
    unsigned char garbage[64] = {0};
    snd_buffer_t  buf         = {0};
    buf.data                  = garbage;
    buf.size                  = sizeof(garbage);

    snd_pe_parser_t pe = {0};
    CHECK(SND_FAILED(snd_pe_parse(&buf, FALSE, &pe)));

    snd_coff_parser_t coff = {0};
    CHECK(SND_FAILED(snd_coff_parse(&buf, &coff)));
}

int main(void) {
    PROGRESS("begin");
    PROGRESS("status encoding");
    test_status_encoding();
    PROGRESS("memory bounds");
    test_memory_bounds();
    PROGRESS("range macros");
    test_range_macros();
    PROGRESS("string helpers");
    test_string_helpers();
    PROGRESS("hash determinism");
    test_hash_determinism();
    PROGRESS("buffer lifecycle");
    test_buffer_lifecycle();
    PROGRESS("parsers reject garbage");
    test_parsers_reject_garbage();
    snd_run_guard_tests();
    PROGRESS("guards complete");

    if (g_test_failures != 0) {
        fprintf(stderr, "unit tests: %d failure(s)\n", g_test_failures);
        fflush(stderr);
        return 1;
    }

    printf("unit tests: all passed\n");
    fflush(stdout);
    PROGRESS("main returning");
    return 0;
}

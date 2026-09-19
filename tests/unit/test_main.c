#include "test_util.h"

static int g_freed = 0;

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

static void test_status_strings(void) {
    /* snd_status_to_string must never return NULL and must classify success
     * and a couple of failure facilities without crashing. */
    CHECK(snd_status_to_string(SND_OK) != NULL);
    CHECK(snd_status_to_string(SND_ERR(SND_STATUS_NULL_POINTER)) != NULL);
    CHECK(snd_status_to_string(SND_ERR(SND_STATUS_SSN_NOT_FOUND)) != NULL);
    CHECK(snd_status_to_string(SND_ERR(SND_STATUS_HEADER_NT_SIGNATURE_INVALID)) != NULL);
    CHECK(snd_status_to_string(SND_ERR(SND_STATUS_HEADER_MACHINE_UNSUPPORTED)) != NULL);
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
    SIZE_T total  = 16;
    SIZE_T offset = 8;
    SIZE_T length = 8;

    CHECK(SND_RANGE_EXCEEDS(offset, length, total) == 0);
    length = 9;
    CHECK(SND_RANGE_EXCEEDS(offset, length, total) != 0);
    CHECK(SND_IN_BOUNDS(15, 0, total) != 0);
    CHECK(SND_IN_BOUNDS(total, 0, total) == 0);
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

    g_freed = 0;
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

void snd_test_register_common(void) {
    snd_test_register("common: status encoding", test_status_encoding);
    snd_test_register("common: status strings", test_status_strings);
    snd_test_register("common: memory bounds", test_memory_bounds);
    snd_test_register("common: range macros", test_range_macros);
    snd_test_register("common: string helpers", test_string_helpers);
    snd_test_register("common: hash determinism", test_hash_determinism);
    snd_test_register("common: buffer lifecycle", test_buffer_lifecycle);
    snd_test_register("common: parsers reject garbage", test_parsers_reject_garbage);
}

int main(int argc, char **argv) {
    snd_test_register_common();
    snd_test_register_guards();
    snd_test_register_hijack();
    snd_test_register_pe_parser();
    snd_test_register_pe_exports();
    snd_test_register_pe_imports();
    snd_test_register_pe_relocs();
    snd_test_register_coff_parser();
    snd_test_register_coff_symbols();
    snd_test_register_inject_chains();

    return snd_test_main(argc, argv);
}

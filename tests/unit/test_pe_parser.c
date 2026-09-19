#include "pe_builder.h"
#include "test_util.h"

static int build_image(snd_test_pe_t *img, const snd_test_pe_spec_t *spec) {
    if (snd_test_pe_build(img, spec) != 0) {
        return -1;
    }
    return snd_test_pe_finalize(img);
}

static void test_parse_happy_path_raw64(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1, .is_mapped = 0};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    snd_status_t    st  = snd_pe_parse(&buf, FALSE, &pe);
    SND_CHECK_SUCCEEDED(st);
    CHECK(pe.is_64bit == TRUE);
    CHECK(pe.is_mapped == FALSE);
    CHECK(pe.is_dll == FALSE);
    CHECK_EQ_INT(pe.sections_count, 1);
    CHECK(pe.dos != NULL);
    CHECK(pe.nt.nt64 != NULL);
    CHECK(pe.section_head != NULL);
    CHECK_EQ_U32(pe.lfanew, SND_TEST_PE_LFANEW);

    snd_test_pe_free(&img);
}

static void test_parse_happy_path_raw32(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 0, .is_mapped = 0};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    snd_status_t    st  = snd_pe_parse(&buf, FALSE, &pe);
    SND_CHECK_SUCCEEDED(st);
    CHECK(pe.is_64bit == FALSE);
    CHECK(pe.nt.nt32 != NULL);
    CHECK(pe.is_dll == FALSE);

    snd_test_pe_free(&img);
}

static void test_parse_happy_path_mapped64(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1, .is_mapped = 1};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    snd_status_t    st  = snd_pe_parse(&buf, TRUE, &pe);
    SND_CHECK_SUCCEEDED(st);
    CHECK(pe.is_mapped == TRUE);
    CHECK(pe.is_64bit == TRUE);

    snd_test_pe_free(&img);
}

static void test_parse_dll_flag(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1, .is_mapped = 0, .is_dll = 1};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));
    CHECK(pe.is_dll == TRUE);

    snd_test_pe_free(&img);
}

static void test_parse_null_args(void) {
    snd_buffer_t    buf = {0};
    snd_pe_parser_t pe  = {0};
    SND_CHECK_STATUS(snd_pe_parse(NULL, FALSE, &pe), SND_STATUS_NULL_POINTER);
    SND_CHECK_STATUS(snd_pe_parse(&buf, FALSE, NULL), SND_STATUS_NULL_POINTER);
}

static void test_parse_truncated_dos(void) {
    /* Smaller than SND_IMAGE_DOS_HEADER (0x40). */
    unsigned char   small[0x20] = {0};
    snd_buffer_t    buf         = {.data = small, .size = sizeof(small)};
    snd_pe_parser_t pe          = {0};
    SND_CHECK_STATUS(snd_pe_parse(&buf, FALSE, &pe), SND_STATUS_HEADER_DOS_TRUNCATED);
}

static void test_parse_bad_mz(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);
    snd_test_put16(snd_test_pe_data(&img), 0, 0xFFFF); /* clobber MZ */

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_STATUS(snd_pe_parse(&buf, FALSE, &pe), SND_STATUS_HEADER_DOS_SIGNATURE_INVALID);

    snd_test_pe_free(&img);
}

static void test_parse_negative_lfanew(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);
    snd_test_put32(snd_test_pe_data(&img), 0x3C, 0x80000000u); /* LONG < 0 */

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_STATUS(snd_pe_parse(&buf, FALSE, &pe), SND_STATUS_HEADER_OFFSET_INVALID);

    snd_test_pe_free(&img);
}

static void test_parse_lfanew_out_of_bounds(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);
    snd_test_put32(snd_test_pe_data(&img), 0x3C, (DWORD)(snd_test_pe_size(&img) + 0x1000));

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_STATUS(snd_pe_parse(&buf, FALSE, &pe), SND_STATUS_HEADER_NT_TRUNCATED);

    snd_test_pe_free(&img);
}

static void test_parse_bad_nt_signature(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);
    snd_test_put32(snd_test_pe_data(&img), SND_TEST_PE_LFANEW, 0xDEADBEEF);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_STATUS(snd_pe_parse(&buf, FALSE, &pe), SND_STATUS_HEADER_NT_SIGNATURE_INVALID);

    snd_test_pe_free(&img);
}

static void test_parse_bad_optional_magic(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);
    /* Optional header Magic is at 0x80 + 4 + 20. */
    snd_test_put16(snd_test_pe_data(&img), SND_TEST_PE_LFANEW + sizeof(DWORD) + sizeof(SND_IMAGE_FILE_HEADER), 0x9999);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_STATUS(snd_pe_parse(&buf, FALSE, &pe), SND_STATUS_HEADER_OPTIONAL_SIGNATURE_INVALID);

    snd_test_pe_free(&img);
}

static void test_parse_truncated_nt_headers(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);
    /* Cut the buffer right after the file header: e_lfanew + 4 + 20 + 2. */
    size_t cut = SND_TEST_PE_LFANEW + sizeof(DWORD) + sizeof(SND_IMAGE_FILE_HEADER) + sizeof(WORD);

    snd_buffer_t    buf = {.data = snd_test_pe_data(&img), .size = cut};
    snd_pe_parser_t pe  = {0};
    SND_CHECK_STATUS(snd_pe_parse(&buf, FALSE, &pe), SND_STATUS_HEADER_NT_TRUNCATED);

    snd_test_pe_free(&img);
}

static void test_parse_entry_point(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1, .entry_point_rva = SND_TEST_PE_SEC_BASE + 0x10};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);
    /* Provide one byte of section payload so the EP RVA resolves. */
    unsigned char one = 0xC3;
    (void)snd_test_pe_add_region(&img, &one, sizeof(one));
    CHECK_EQ_INT(snd_test_pe_finalize(&img), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));
    CHECK(snd_pe_get_entry_point(&pe) != NULL);

    snd_test_pe_free(&img);
}

static void test_parse_tls_callbacks(void) {
    /* 64-bit mapped image with a TLS directory pointing at a callback array.
     * For mapped images pe_va_to_rva resolves VAs against the live buffer
     * pointer, so AddressOfCallBacks must encode the real data pointer. */
    snd_test_pe_spec_t spec = {.is_64bit = 1, .is_mapped = 1};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);

    /* Callback array: one entry + null terminator (value patched later). */
    ULONGLONG            cb_array[2] = {0, 0};
    snd_test_pe_region_t cb          = snd_test_pe_add_region(&img, cb_array, sizeof(cb_array));

    /* TLS directory struct (64-bit); AddressOfCallBacks patched below. */
    unsigned char        tls[sizeof(SND_IMAGE_TLS_DIRECTORY64)] = {0};
    snd_test_pe_region_t tlsr                                   = snd_test_pe_add_region(&img, tls, sizeof(tls));
    snd_test_pe_set_directory(&img, SND_IMAGE_DIRECTORY_ENTRY_TLS, &tlsr);
    CHECK_EQ_INT(snd_test_pe_finalize(&img), 0);

    /* Patch in the self-relative VA now that the buffer address is fixed. */
    unsigned char *data = snd_test_pe_data(&img);
    snd_test_put64(data, cb.rva, (ULONGLONG)(ULONG_PTR)(data + cb.rva + 0x10));
    snd_test_put64(data, tlsr.rva + offsetof(SND_IMAGE_TLS_DIRECTORY64, AddressOfCallBacks),
                   (ULONGLONG)(ULONG_PTR)data + cb.rva);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, TRUE, &pe));
    CHECK(snd_pe_get_tls_callbacks(&pe) != NULL);

    snd_test_pe_free(&img);
}

void snd_test_register_pe_parser(void) {
    snd_test_register("pe parser: happy path raw PE32+", test_parse_happy_path_raw64);
    snd_test_register("pe parser: happy path raw PE32", test_parse_happy_path_raw32);
    snd_test_register("pe parser: happy path mapped PE32+", test_parse_happy_path_mapped64);
    snd_test_register("pe parser: dll flag", test_parse_dll_flag);
    snd_test_register("pe parser: null args", test_parse_null_args);
    snd_test_register("pe parser: truncated DOS header", test_parse_truncated_dos);
    snd_test_register("pe parser: invalid MZ signature", test_parse_bad_mz);
    snd_test_register("pe parser: negative e_lfanew", test_parse_negative_lfanew);
    snd_test_register("pe parser: e_lfanew out of bounds", test_parse_lfanew_out_of_bounds);
    snd_test_register("pe parser: invalid NT signature", test_parse_bad_nt_signature);
    snd_test_register("pe parser: invalid optional magic", test_parse_bad_optional_magic);
    snd_test_register("pe parser: truncated NT headers", test_parse_truncated_nt_headers);
    snd_test_register("pe parser: entry point resolution", test_parse_entry_point);
    snd_test_register("pe parser: TLS callbacks", test_parse_tls_callbacks);
}
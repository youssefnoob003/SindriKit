#include "pe_builder.h"
#include "test_util.h"

typedef struct {
    snd_test_pe_t        img;
    snd_test_pe_region_t descs; /* descriptors incl. null terminator */
    snd_test_pe_region_t int0;  /* INT for descriptor 0            */
    snd_test_pe_region_t iat0;  /* IAT for descriptor 0            */
    snd_test_pe_region_t int1;  /* INT for descriptor 1            */
    snd_test_pe_region_t iat1;  /* IAT for descriptor 1            */
    snd_test_pe_region_t names; /* IMAGE_IMPORT_BY_NAME structs    */
    snd_test_pe_region_t dll0;
    snd_test_pe_region_t dll1;
} snd_test_import_ctx_t;

static size_t thunk_stride(int is_64bit) {
    return is_64bit ? sizeof(SND_IMAGE_THUNK_DATA64) : sizeof(SND_IMAGE_THUNK_DATA32);
}

static int build_image(snd_test_pe_t *img, const snd_test_pe_spec_t *spec) {
    if (snd_test_pe_build(img, spec) != 0) {
        return -1;
    }
    return snd_test_pe_finalize(img);
}

static void put_thunk(unsigned char *p, size_t off, int is_64bit, ULONGLONG value) {
    if (is_64bit) {
        snd_test_put64(p, off, value);
    } else {
        snd_test_put32(p, off, (DWORD)value);
    }
}

static int build_import_ctx(snd_test_import_ctx_t *c, int is_64bit) {
    memset(c, 0, sizeof(*c));

    snd_test_pe_spec_t spec = {.is_64bit = is_64bit};
    if (snd_test_pe_build(&c->img, &spec) != 0) {
        return -1;
    }

    size_t stride = thunk_stride(is_64bit);

    /* Import-by-name structs. */
    unsigned char names[64] = {0};
    /* struct0: Hint=0, "ReadFile" */
    snd_test_put16(names, 0, 0);
    memcpy(names + 2, "ReadFile", 9);
    /* struct1: Hint=0, "MessageBoxA" */
    size_t s1 = 2 + 9;
    snd_test_put16(names, s1, 0);
    memcpy(names + s1 + 2, "MessageBoxA", 12);
    c->names = snd_test_pe_add_region(&c->img, names, sizeof(names));

    /* INT/IAT for descriptor 0: by-name (ReadFile), by-ordinal (7), end. */
    unsigned char int0[3 * 8] = {0};
    unsigned char iat0[3 * 8] = {0};
    put_thunk(int0, 0 * stride, is_64bit, c->names.rva);
    put_thunk(int0, 1 * stride, is_64bit, (ULONGLONG)(is_64bit ? 0x8000000000000000ull : 0x80000000u) | 7u);
    put_thunk(iat0, 0 * stride, is_64bit, c->names.rva);
    put_thunk(iat0, 1 * stride, is_64bit, 0xDEAD0000ull);
    c->int0 = snd_test_pe_add_region(&c->img, int0, 3 * stride);
    c->iat0 = snd_test_pe_add_region(&c->img, iat0, 3 * stride);

    /* INT/IAT for descriptor 1: IAT-only binding (OFT == 0), by-name. The
     * thunk value is the RVA of the IMAGE_IMPORT_BY_NAME struct (Hint at +0,
     * Name at +2); the parser reads Name from struct+2. */
    unsigned char int1[2 * 8] = {0};
    unsigned char iat1[2 * 8] = {0};
    put_thunk(int1, 0 * stride, is_64bit, c->names.rva + s1);
    put_thunk(iat1, 0 * stride, is_64bit, c->names.rva + s1);
    c->int1 = snd_test_pe_add_region(&c->img, int1, 2 * stride);
    c->iat1 = snd_test_pe_add_region(&c->img, iat1, 2 * stride);

    /* DLL names. */
    c->dll0 = snd_test_pe_add_region(&c->img, "kernel32.dll", 13);
    c->dll1 = snd_test_pe_add_region(&c->img, "user32.dll", 11);

    /* Descriptors (2 + terminator). */
    unsigned char descs[3 * sizeof(SND_IMAGE_IMPORT_DESCRIPTOR)] = {0};
    snd_test_put32(descs, 0, c->int0.rva); /* OriginalFirstThunk */
    snd_test_put32(descs, 4, 0);
    snd_test_put32(descs, 8, 0);
    snd_test_put32(descs, 12, c->dll0.rva); /* Name */
    snd_test_put32(descs, 16, c->iat0.rva); /* FirstThunk */
    snd_test_put32(descs, 20, 0);           /* desc1: OFT == 0 (IAT-only) */
    snd_test_put32(descs, 24, 0);
    snd_test_put32(descs, 28, 0);
    snd_test_put32(descs, 32, c->dll1.rva);
    snd_test_put32(descs, 36, c->iat1.rva);
    /* desc2: terminator, all zero. */
    c->descs = snd_test_pe_add_region(&c->img, descs, sizeof(descs));

    snd_test_pe_set_directory(&c->img, SND_IMAGE_DIRECTORY_ENTRY_IMPORT, &c->descs);
    return snd_test_pe_finalize(&c->img);
}

static void test_import_descriptors(void) {
    snd_test_import_ctx_t c = {0};
    CHECK_EQ_INT(build_import_ctx(&c, 1), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    const SND_IMAGE_IMPORT_DESCRIPTOR *d = NULL;
    SND_CHECK_SUCCEEDED(snd_pe_get_import_descriptor(&pe, 0, &d));
    CHECK(d != NULL);

    const char *name = NULL;
    SND_CHECK_SUCCEEDED(snd_pe_get_import_name(&pe, d, &name));
    SND_CHECK_STR_EQ(name, "kernel32.dll");

    SND_CHECK_SUCCEEDED(snd_pe_get_import_descriptor(&pe, 1, &d));
    CHECK(d != NULL);
    SND_CHECK_SUCCEEDED(snd_pe_get_import_name(&pe, d, &name));
    SND_CHECK_STR_EQ(name, "user32.dll");

    /* Terminator and out-of-range must both return SND_OK with NULL. */
    SND_CHECK_SUCCEEDED(snd_pe_get_import_descriptor(&pe, 2, &d));
    CHECK(d == NULL);
    SND_CHECK_SUCCEEDED(snd_pe_get_import_descriptor(&pe, 99, &d));
    CHECK(d == NULL);

    snd_test_pe_free(&c.img);
}

static void test_import_thunks(void) {
    snd_test_import_ctx_t c = {0};
    CHECK_EQ_INT(build_import_ctx(&c, 1), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    const SND_IMAGE_IMPORT_DESCRIPTOR *d = NULL;
    SND_CHECK_SUCCEEDED(snd_pe_get_import_descriptor(&pe, 0, &d));

    snd_pe_import_thunk_t t = {0};
    SND_CHECK_SUCCEEDED(snd_pe_get_import_thunk(&pe, d, 0, &t));
    CHECK(t.is_ordinal == FALSE);
    CHECK(t.iat_slot != NULL);
    SND_CHECK_STR_EQ(t.name, "ReadFile");

    SND_CHECK_SUCCEEDED(snd_pe_get_import_thunk(&pe, d, 1, &t));
    CHECK(t.is_ordinal == TRUE);
    CHECK_EQ_U32(t.ordinal, 7);

    /* Null terminator thunk. */
    SND_CHECK_SUCCEEDED(snd_pe_get_import_thunk(&pe, d, 2, &t));
    CHECK(t.iat_slot == NULL);
    CHECK(t.name == NULL);

    /* Index far past the thunk array must fail bounds. */
    SND_CHECK_STATUS(snd_pe_get_import_thunk(&pe, d, 99, &t), SND_STATUS_IMPORT_THUNK_OUT_OF_BOUNDS);

    snd_test_pe_free(&c.img);
}

static void test_import_iat_only_binding(void) {
    /* Descriptor 1 binds with OriginalFirstThunk == 0; the name must still be
     * recovered through FirstThunk. */
    snd_test_import_ctx_t c = {0};
    CHECK_EQ_INT(build_import_ctx(&c, 1), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    const SND_IMAGE_IMPORT_DESCRIPTOR *d = NULL;
    SND_CHECK_SUCCEEDED(snd_pe_get_import_descriptor(&pe, 1, &d));

    snd_pe_import_thunk_t t = {0};
    SND_CHECK_SUCCEEDED(snd_pe_get_import_thunk(&pe, d, 0, &t));
    CHECK(t.is_ordinal == FALSE);
    SND_CHECK_STR_EQ(t.name, "MessageBoxA");

    snd_test_pe_free(&c.img);
}

static void test_import_invalid_name(void) {
    snd_test_import_ctx_t c = {0};
    CHECK_EQ_INT(build_import_ctx(&c, 1), 0);

    /* Zero desc0.Name after build. */
    size_t off = (size_t)(c.descs.rva - SND_TEST_PE_SEC_BASE) + 12;
    memset(snd_test_pe_data(&c.img) + c.img.section_off + off, 0, sizeof(DWORD));

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    const SND_IMAGE_IMPORT_DESCRIPTOR *d = NULL;
    SND_CHECK_SUCCEEDED(snd_pe_get_import_descriptor(&pe, 0, &d));
    const char *name = NULL;
    SND_CHECK_STATUS(snd_pe_get_import_name(&pe, d, &name), SND_STATUS_IMPORT_NAME_INVALID);

    snd_test_pe_free(&c.img);
}

static void test_import_bad_directory_rva(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);

    /* Point the import directory at unmapped memory. */
    snd_test_pe_set_directory_rva(&img, SND_IMAGE_DIRECTORY_ENTRY_IMPORT, 0xDEAD0000u, 20);
    CHECK_EQ_INT(snd_test_pe_finalize(&img), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    const SND_IMAGE_IMPORT_DESCRIPTOR *d = NULL;
    SND_CHECK_STATUS(snd_pe_get_import_descriptor(&pe, 0, &d), SND_STATUS_IMPORT_DESCRIPTOR_INVALID);

    snd_test_pe_free(&img);
}

static void test_import_32bit_thunks(void) {
    snd_test_import_ctx_t c = {0};
    CHECK_EQ_INT(build_import_ctx(&c, 0), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    const SND_IMAGE_IMPORT_DESCRIPTOR *d = NULL;
    SND_CHECK_SUCCEEDED(snd_pe_get_import_descriptor(&pe, 0, &d));

    snd_pe_import_thunk_t t = {0};
    SND_CHECK_SUCCEEDED(snd_pe_get_import_thunk(&pe, d, 0, &t));
    CHECK(t.is_ordinal == FALSE);
    SND_CHECK_STR_EQ(t.name, "ReadFile");

    SND_CHECK_SUCCEEDED(snd_pe_get_import_thunk(&pe, d, 1, &t));
    CHECK(t.is_ordinal == TRUE);
    CHECK_EQ_U32(t.ordinal, 7);

    snd_test_pe_free(&c.img);
}

void snd_test_register_pe_imports(void) {
    snd_test_register("pe imports: descriptors", test_import_descriptors);
    snd_test_register("pe imports: thunks", test_import_thunks);
    snd_test_register("pe imports: IAT-only binding", test_import_iat_only_binding);
    snd_test_register("pe imports: invalid name", test_import_invalid_name);
    snd_test_register("pe imports: bad directory RVA", test_import_bad_directory_rva);
    snd_test_register("pe imports: 32-bit thunks", test_import_32bit_thunks);
}
#include "pe_builder.h"
#include "test_util.h"

#include <stdlib.h>
#include <string.h>

#define SND_TEST_EXP_BASE  1
#define SND_TEST_EXP_NFUNC 3
#define SND_TEST_EXP_NNAME 2

typedef struct {
    snd_test_pe_t        img;
    snd_test_pe_region_t dir;   /* export struct + strings   */
    snd_test_pe_region_t funcs; /* AddressOfFunctions        */
    snd_test_pe_region_t names; /* AddressOfNames            */
    snd_test_pe_region_t ords;  /* AddressOfNameOrdinals     */
    snd_test_pe_region_t code;  /* function bodies           */
} snd_test_export_ctx_t;

/* Placeholder regions added in order: dir blob, funcs, names, ords. The dir
 * blob (struct + strings + padding) is patched in place after the arrays that
 * reference it are placed, avoiding any RVA prediction. */
static void put_at_rva(snd_test_pe_t *img, DWORD rva, size_t off, const void *src, size_t len) {
    size_t sec_off = (size_t)(rva - SND_TEST_PE_SEC_BASE);
    memcpy(img->mem.data + img->section_off + sec_off + off, src, len);
}

static void put32_at_rva(snd_test_pe_t *img, DWORD rva, size_t off, DWORD v) {
    unsigned char tmp[4];
    snd_test_put32(tmp, 0, v);
    put_at_rva(img, rva, off, tmp, sizeof(tmp));
}

static int build_export_ctx(snd_test_export_ctx_t *c, int is_64bit, int is_mapped) {
    memset(c, 0, sizeof(*c));

    snd_test_pe_spec_t spec = {.is_64bit = is_64bit, .is_mapped = is_mapped};
    if (snd_test_pe_build(&c->img, &spec) != 0) {
        return -1;
    }

    /* Function bodies. */
    unsigned char bodies[16] = {0xC3, 0xC3, 0xC3};
    c->code                  = snd_test_pe_add_region(&c->img, bodies, sizeof(bodies));

    /* Export directory region layout:
     *   [struct][dll name][name A][name B][forwarder string][padding] */
    const char *dll = "snd_test.dll";
    const char *n1  = "SayHello";
    const char *n2  = "FwdEntry";
    /* Windows forwarder syntax is MODULE.#ordinal (or MODULE.FuncName): the dot
     * is the mandatory module/function separator and .dll is appended by the
     * engine. "snd_fwd_target#2" (no dot) is rejected as EXPORT_FORWARDER_INVALID
     * and "snd_fwd_target.dll#2" would name the function "dll#2". */
    const char *fwd      = "snd_fwd_target.#2";
    size_t      off_dll  = sizeof(SND_IMAGE_EXPORT_DIRECTORY);
    size_t      off_n1   = off_dll + strlen(dll) + 1;
    size_t      off_n2   = off_n1 + strlen(n1) + 1;
    size_t      off_fwd  = off_n2 + strlen(n2) + 1;
    size_t      blob_len = off_fwd + strlen(fwd) + 1 + 8; /* padding so the forwarder
                                                             string is provably NUL-terminated */

    unsigned char *blob = (unsigned char *)calloc(blob_len, 1);
    if (blob == NULL) {
        return -1;
    }
    memcpy(blob + off_dll, dll, strlen(dll) + 1);
    memcpy(blob + off_n1, n1, strlen(n1) + 1);
    memcpy(blob + off_n2, n2, strlen(n2) + 1);
    memcpy(blob + off_fwd, fwd, strlen(fwd) + 1);

    c->dir = snd_test_pe_add_region(&c->img, blob, blob_len);
    free(blob);

    /* AddressOfFunctions: [SayHello][forwarder][unused]. */
    DWORD funcs[SND_TEST_EXP_NFUNC] = {c->code.rva, c->dir.rva + (DWORD)off_fwd, 0};
    c->funcs                        = snd_test_pe_add_region(&c->img, funcs, sizeof(funcs));

    /* AddressOfNames / NameOrdinals. */
    DWORD names[SND_TEST_EXP_NNAME] = {c->dir.rva + (DWORD)off_n1, c->dir.rva + (DWORD)off_n2};
    WORD  ords[SND_TEST_EXP_NNAME]  = {0, 1};
    c->names                        = snd_test_pe_add_region(&c->img, names, sizeof(names));
    c->ords                         = snd_test_pe_add_region(&c->img, ords, sizeof(ords));

    /* Patch the export struct in place. */
    put32_at_rva(&c->img, c->dir.rva, offsetof(SND_IMAGE_EXPORT_DIRECTORY, Base), SND_TEST_EXP_BASE);
    put32_at_rva(&c->img, c->dir.rva, offsetof(SND_IMAGE_EXPORT_DIRECTORY, NumberOfFunctions), SND_TEST_EXP_NFUNC);
    put32_at_rva(&c->img, c->dir.rva, offsetof(SND_IMAGE_EXPORT_DIRECTORY, NumberOfNames), SND_TEST_EXP_NNAME);
    put32_at_rva(&c->img, c->dir.rva, offsetof(SND_IMAGE_EXPORT_DIRECTORY, Name), c->dir.rva + (DWORD)off_dll);
    put32_at_rva(&c->img, c->dir.rva, offsetof(SND_IMAGE_EXPORT_DIRECTORY, AddressOfFunctions), c->funcs.rva);
    put32_at_rva(&c->img, c->dir.rva, offsetof(SND_IMAGE_EXPORT_DIRECTORY, AddressOfNames), c->names.rva);
    put32_at_rva(&c->img, c->dir.rva, offsetof(SND_IMAGE_EXPORT_DIRECTORY, AddressOfNameOrdinals), c->ords.rva);

    snd_test_pe_set_directory(&c->img, SND_IMAGE_DIRECTORY_ENTRY_EXPORT, &c->dir);
    return snd_test_pe_finalize(&c->img);
}

/* A minimal mapped module (64- or 32-bit) that exports two functions so an
 * ordinal forwarder ("#2") resolves against it. The buffer is exactly
 * SND_SYS_DLL_SIZE_DEFAULT so the engine's forwarder path can parse it. */
static snd_test_pe_t g_fwd_target;

static int build_fwd_target(int is_64bit) {
    snd_test_pe_spec_t spec = {.is_64bit = is_64bit, .is_mapped = 1};
    if (snd_test_pe_build(&g_fwd_target, &spec) != 0) {
        return -1;
    }

    unsigned char        stub[4] = {0xC3};
    snd_test_pe_region_t code    = snd_test_pe_add_region(&g_fwd_target, stub, sizeof(stub));

    DWORD                funcs[2] = {code.rva, code.rva};
    snd_test_pe_region_t f        = snd_test_pe_add_region(&g_fwd_target, funcs, sizeof(funcs));

    unsigned char dir[sizeof(SND_IMAGE_EXPORT_DIRECTORY)] = {0};
    snd_test_put32(dir, offsetof(SND_IMAGE_EXPORT_DIRECTORY, Base), 1);
    snd_test_put32(dir, offsetof(SND_IMAGE_EXPORT_DIRECTORY, NumberOfFunctions), 2);
    snd_test_put32(dir, offsetof(SND_IMAGE_EXPORT_DIRECTORY, NumberOfNames), 0);
    snd_test_put32(dir, offsetof(SND_IMAGE_EXPORT_DIRECTORY, AddressOfFunctions), f.rva);
    snd_test_pe_region_t dr = snd_test_pe_add_region(&g_fwd_target, dir, sizeof(dir));
    snd_test_pe_set_directory(&g_fwd_target, SND_IMAGE_DIRECTORY_ENTRY_EXPORT, &dr);

    return snd_test_pe_finalize(&g_fwd_target);
}

static snd_status_t WINAPI test_module_resolver(const wchar_t *module_name, PVOID *out_base) {
    (void)module_name;
    *out_base = g_fwd_target.mem.data;
    return SND_OK;
}

static void test_export_by_name(void) {
    snd_test_export_ctx_t c = {0};
    CHECK_EQ_INT(build_export_ctx(&c, 1, 0), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    FARPROC addr = NULL;
    SND_CHECK_SUCCEEDED(snd_pe_get_export_address(&pe, "SayHello", &addr, NULL));
    CHECK(addr != NULL);

    FARPROC addr2 = NULL;
    SND_CHECK_SUCCEEDED(snd_pe_get_export_address(&pe, "SayHello", &addr2, NULL));
    CHECK(addr == addr2);

    snd_test_pe_free(&c.img);
}

static void test_export_by_hash(void) {
    snd_test_export_ctx_t c = {0};
    CHECK_EQ_INT(build_export_ctx(&c, 1, 0), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    FARPROC by_name = NULL;
    FARPROC by_hash = NULL;
    SND_CHECK_SUCCEEDED(snd_pe_get_export_address(&pe, "SayHello", &by_name, NULL));
    SND_CHECK_SUCCEEDED(snd_pe_get_export_address_hash(&pe, snd_hash("SayHello"), &by_hash, NULL));
    CHECK(by_hash != NULL);
    CHECK(by_name == by_hash);

    snd_test_pe_free(&c.img);
}

static void test_export_by_ordinal(void) {
    snd_test_export_ctx_t c = {0};
    CHECK_EQ_INT(build_export_ctx(&c, 1, 0), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    FARPROC by_name = NULL;
    FARPROC by_ord  = NULL;
    SND_CHECK_SUCCEEDED(snd_pe_get_export_address(&pe, "SayHello", &by_name, NULL));
    SND_CHECK_SUCCEEDED(snd_pe_get_export_address(&pe, SND_MAKEINTRESOURCE(1), &by_ord, NULL));
    CHECK(by_name == by_ord);

    /* Ordinal 3 has a zero AddressOfFunctions slot. */
    SND_CHECK_STATUS(snd_pe_get_export_address(&pe, SND_MAKEINTRESOURCE(3), &by_ord, NULL),
                     SND_STATUS_EXPORT_SYMBOL_MISSING);
    /* Out of range. */
    SND_CHECK_STATUS(snd_pe_get_export_address(&pe, SND_MAKEINTRESOURCE(99), &by_ord, NULL),
                     SND_STATUS_EXPORT_ORDINAL_OUT_OF_RANGE);
    /* MAKEINTRESOURCE(0) is NULL; the engine rejects a NULL name/ordinal as an
     * invalid parameter combination (its ordinal==0 check is unreachable via
     * the public API). */
    SND_CHECK_STATUS(snd_pe_get_export_address(&pe, SND_MAKEINTRESOURCE(0), &by_ord, NULL),
                     SND_STATUS_INVALID_PARAMETERS_COMBINATION);

    snd_test_pe_free(&c.img);
}

static void test_export_missing_symbol(void) {
    snd_test_export_ctx_t c = {0};
    CHECK_EQ_INT(build_export_ctx(&c, 1, 0), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    FARPROC addr = NULL;
    SND_CHECK_STATUS(snd_pe_get_export_address(&pe, "NoSuchExport", &addr, NULL), SND_STATUS_EXPORT_SYMBOL_MISSING);

    snd_test_pe_free(&c.img);
}

static void test_export_null_args(void) {
    snd_test_export_ctx_t c = {0};
    CHECK_EQ_INT(build_export_ctx(&c, 1, 0), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    FARPROC addr = NULL;
    SND_CHECK_STATUS(snd_pe_get_export_address(NULL, "SayHello", &addr, NULL), SND_STATUS_NULL_POINTER);
    SND_CHECK_STATUS(snd_pe_get_export_address(&pe, NULL, &addr, NULL), SND_STATUS_INVALID_PARAMETERS_COMBINATION);
    SND_CHECK_STATUS(snd_pe_get_export_address(&pe, "SayHello", NULL, NULL), SND_STATUS_NULL_POINTER);
    SND_CHECK_STATUS(snd_pe_get_export_address_hash(&pe, 0, &addr, NULL), SND_STATUS_INVALID_PARAMETERS_COMBINATION);
    SND_CHECK_STATUS(snd_pe_get_export_address_hash(&pe, snd_hash("SayHello"), NULL, NULL), SND_STATUS_NULL_POINTER);

    snd_test_pe_free(&c.img);
}

static int build_image_local(snd_test_pe_t *img, const snd_test_pe_spec_t *spec) {
    if (snd_test_pe_build(img, spec) != 0) {
        return -1;
    }
    return snd_test_pe_finalize(img);
}

static void test_export_missing_directory(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image_local(&img, &spec), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    FARPROC addr = NULL;
    SND_CHECK_STATUS(snd_pe_get_export_address(&pe, "Anything", &addr, NULL), SND_STATUS_DIRECTORY_ENTRY_MISSING);

    snd_test_pe_free(&img);
}

static void test_export_forwarder(void) {
    if (build_fwd_target(1) != 0) {
        CHECK(FALSE);
        return;
    }

    snd_test_export_ctx_t c = {0};
    CHECK_EQ_INT(build_export_ctx(&c, 1, 0), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    /* Without a resolver the forwarder must be rejected. */
    FARPROC addr = NULL;
    SND_CHECK_STATUS(snd_pe_get_export_address(&pe, "FwdEntry", &addr, NULL), SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED);

    /* With the stub resolver it must resolve through the target module. */
    SND_CHECK_SUCCEEDED(snd_pe_get_export_address(&pe, "FwdEntry", &addr, test_module_resolver));
    CHECK(addr != NULL);

    snd_test_pe_free(&c.img);
    snd_test_pe_free(&g_fwd_target);
}

typedef struct {
    const char *names[8];
    WORD        ords[8];
    PVOID       addrs[8];
    int         count;
} snd_test_enum_ctx_t;

static BOOL collect_export(const char *func_name, WORD ordinal, PVOID func_addr, PVOID user_ctx) {
    snd_test_enum_ctx_t *c = (snd_test_enum_ctx_t *)user_ctx;
    if (c->count < 8) {
        c->names[c->count] = func_name;
        c->ords[c->count]  = ordinal;
        c->addrs[c->count] = func_addr;
        c->count++;
    }
    return TRUE;
}

static void test_export_enumerate(void) {
    snd_test_export_ctx_t c = {0};
    CHECK_EQ_INT(build_export_ctx(&c, 1, 0), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    snd_test_enum_ctx_t e = {0};
    SND_CHECK_SUCCEEDED(snd_pe_enumerate_exports(&pe, collect_export, &e));
    CHECK_EQ_INT(e.count, 2);
    SND_CHECK_STR_EQ(e.names[0], "SayHello");
    SND_CHECK_STR_EQ(e.names[1], "FwdEntry");
    CHECK_EQ_U32(e.ords[0], SND_TEST_EXP_BASE + 0);
    CHECK_EQ_U32(e.ords[1], SND_TEST_EXP_BASE + 1);
    CHECK(e.addrs[0] != NULL);
    CHECK(e.addrs[1] != NULL);

    snd_test_pe_free(&c.img);
}

static void test_export_32bit(void) {
    snd_test_export_ctx_t c = {0};
    CHECK_EQ_INT(build_export_ctx(&c, 0, 0), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&c.img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    FARPROC addr = NULL;
    SND_CHECK_SUCCEEDED(snd_pe_get_export_address(&pe, "SayHello", &addr, NULL));
    CHECK(addr != NULL);
    SND_CHECK_SUCCEEDED(snd_pe_get_export_address(&pe, SND_MAKEINTRESOURCE(1), &addr, NULL));
    CHECK(addr != NULL);

    snd_test_pe_free(&c.img);
}

void snd_test_register_pe_exports(void) {
    snd_test_register("pe exports: by name", test_export_by_name);
    snd_test_register("pe exports: by hash", test_export_by_hash);
    snd_test_register("pe exports: by ordinal", test_export_by_ordinal);
    snd_test_register("pe exports: missing symbol", test_export_missing_symbol);
    snd_test_register("pe exports: null args", test_export_null_args);
    snd_test_register("pe exports: missing directory", test_export_missing_directory);
    snd_test_register("pe exports: forwarder", test_export_forwarder);
    snd_test_register("pe exports: enumerate", test_export_enumerate);
    snd_test_register("pe exports: 32-bit", test_export_32bit);
}

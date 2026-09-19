#include "coff_builder.h"
#include "test_util.h"

/* Builds an object with:
 *   idx 0: "first"  (SectionNumber=1) with one auxiliary record
 *   idx 2: "third"  (SectionNumber=1)
 *   idx 3: "go"     (SectionNumber=1)
 *   idx 4: "_func"  (SectionNumber=1)
 *   idx 5: "__imp_MSVCRT$puts" (undefined, import; long name)
 *   idx 6: "bss_sym" (undefined, Value != 0)
 *   idx 7: "naked"   (undefined, Value == 0, no '$')
 *   idx 8: ".text"   (SectionNumber = -1) */
static int build_symbol_object(snd_test_coff_t *obj) {
    if (snd_test_coff_build(obj, SND_IMAGE_FILE_MACHINE_AMD64) != 0) {
        return -1;
    }
    CHECK(snd_test_coff_add_section(obj, ".text", 0x60000020u) == 1);

    SND_IMAGE_SYMBOL sym = {0};

    memset(&sym, 0, sizeof(sym));
    memcpy(sym.N.ShortName, "first", 5);
    sym.SectionNumber      = 1;
    sym.NumberOfAuxSymbols = 1;
    CHECK(snd_test_coff_add_symbol(obj, &sym) == 0);
    /* aux record */
    memset(&sym, 0, sizeof(sym));
    CHECK(snd_test_coff_add_symbol(obj, &sym) == 1);

    memset(&sym, 0, sizeof(sym));
    memcpy(sym.N.ShortName, "third", 5);
    sym.SectionNumber = 1;
    CHECK(snd_test_coff_add_symbol(obj, &sym) == 2);

    memset(&sym, 0, sizeof(sym));
    memcpy(sym.N.ShortName, "go", 2);
    sym.SectionNumber = 1;
    CHECK(snd_test_coff_add_symbol(obj, &sym) == 3);

    memset(&sym, 0, sizeof(sym));
    memcpy(sym.N.ShortName, "_func", 5);
    sym.SectionNumber = 1;
    CHECK(snd_test_coff_add_symbol(obj, &sym) == 4);

    /* __imp_MSVCRT$puts stored as a long name. */
    DWORD imp_off = snd_test_coff_add_string(obj, "__imp_MSVCRT$puts");
    CHECK(imp_off != 0);
    memset(&sym, 0, sizeof(sym));
    sym.N.Name.Short  = 0;
    sym.N.Name.Long   = imp_off;
    sym.SectionNumber = 0; /* undefined */
    sym.Value         = 0;
    CHECK(snd_test_coff_add_symbol(obj, &sym) == 5);

    memset(&sym, 0, sizeof(sym));
    memcpy(sym.N.ShortName, "bss_sym", 7);
    sym.SectionNumber = 0;
    sym.Value         = 0x100;
    CHECK(snd_test_coff_add_symbol(obj, &sym) == 6);

    memset(&sym, 0, sizeof(sym));
    memcpy(sym.N.ShortName, "naked", 5);
    sym.SectionNumber = 0;
    sym.Value         = 0;
    CHECK(snd_test_coff_add_symbol(obj, &sym) == 7);

    memset(&sym, 0, sizeof(sym));
    memcpy(sym.N.ShortName, ".text", 5);
    sym.SectionNumber = -1; /* section symbol */
    CHECK(snd_test_coff_add_symbol(obj, &sym) == 8);

    return snd_test_coff_finalize(obj);
}

static void test_coff_find_short_name(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(build_symbol_object(&obj), 0);

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_SUCCEEDED(snd_coff_parse(&buf, &coff));

    PSND_IMAGE_SYMBOL found = NULL;
    DWORD             index = 0;
    SND_CHECK_SUCCEEDED(snd_coff_find_symbol_by_name(&coff, "go", 2, &found, &index));
    CHECK(found != NULL);
    CHECK_EQ_U32(index, 3);

    SND_CHECK_STATUS(snd_coff_find_symbol_by_name(&coff, "missing", 7, &found, &index),
                     SND_STATUS_SYMBOL_ENTRY_MISSING);

    snd_test_coff_free(&obj);
}

static void test_coff_find_long_name(void) {
    /* Separate object: one long-name symbol. */
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(snd_test_coff_build(&obj, SND_IMAGE_FILE_MACHINE_AMD64), 0);
    CHECK(snd_test_coff_add_section(&obj, ".text", 0x60000020u) == 1);

    DWORD off = snd_test_coff_add_string(&obj, "very_long_function_name");
    CHECK(off != 0);

    SND_IMAGE_SYMBOL sym = {0};
    sym.N.Name.Short     = 0;
    sym.N.Name.Long      = off;
    sym.SectionNumber    = 1;
    CHECK(snd_test_coff_add_symbol(&obj, &sym) == 0);
    CHECK_EQ_INT(snd_test_coff_finalize(&obj), 0);

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_SUCCEEDED(snd_coff_parse(&buf, &coff));

    PSND_IMAGE_SYMBOL found = NULL;
    DWORD             index = 0;
    SND_CHECK_SUCCEEDED(snd_coff_find_symbol_by_name(&coff, "very_long_function_name",
                                                     sizeof("very_long_function_name") - 1, &found, &index));
    CHECK(found != NULL);
    CHECK_EQ_U32(index, 0);

    snd_test_coff_free(&obj);
}

static void test_coff_find_underscore_tolerance(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(build_symbol_object(&obj), 0);

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_SUCCEEDED(snd_coff_parse(&buf, &coff));

    /* Symbol stored as "_func" must be found when asked for "func". */
    PSND_IMAGE_SYMBOL found = NULL;
    DWORD             index = 0;
    SND_CHECK_SUCCEEDED(snd_coff_find_symbol_by_name(&coff, "func", 4, &found, &index));
    CHECK(found != NULL);
    CHECK_EQ_U32(index, 4);

    snd_test_coff_free(&obj);
}

static void test_coff_find_skips_aux(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(build_symbol_object(&obj), 0);

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_SUCCEEDED(snd_coff_parse(&buf, &coff));

    PSND_IMAGE_SYMBOL found = NULL;
    DWORD             index = 0;
    SND_CHECK_SUCCEEDED(snd_coff_find_symbol_by_name(&coff, "third", 5, &found, &index));
    CHECK(found != NULL);
    CHECK_EQ_U32(index, 2);

    snd_test_coff_free(&obj);
}

static void test_coff_decode_local(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(build_symbol_object(&obj), 0);

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_SUCCEEDED(snd_coff_parse(&buf, &coff));

    PSND_IMAGE_SYMBOL sym = snd_coff_get_symbol_by_index(&coff, 3);
    CHECK(sym != NULL);

    snd_coff_decoded_sym_t dec = {0};
    SND_CHECK_SUCCEEDED(snd_coff_decode_symbol(&coff, sym, &dec));
    CHECK(dec.type == SND_COFF_SYM_TYPE_LOCAL);

    snd_test_coff_free(&obj);
}

static void test_coff_decode_import(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(build_symbol_object(&obj), 0);

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_SUCCEEDED(snd_coff_parse(&buf, &coff));

    PSND_IMAGE_SYMBOL sym = snd_coff_get_symbol_by_index(&coff, 5);
    CHECK(sym != NULL);

    snd_coff_decoded_sym_t dec = {0};
    SND_CHECK_SUCCEEDED(snd_coff_decode_symbol(&coff, sym, &dec));
    CHECK(dec.type == SND_COFF_SYM_TYPE_IMPORT);
    CHECK(dec.import.is_imp == TRUE);
    SND_CHECK_STR_EQ(dec.import.dll_name, "MSVCRT");
    SND_CHECK_STR_EQ(dec.import.func_name, "puts");

    snd_test_coff_free(&obj);
}

static void test_coff_decode_bss(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(build_symbol_object(&obj), 0);

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_SUCCEEDED(snd_coff_parse(&buf, &coff));

    PSND_IMAGE_SYMBOL sym = snd_coff_get_symbol_by_index(&coff, 6);
    CHECK(sym != NULL);

    snd_coff_decoded_sym_t dec = {0};
    SND_CHECK_SUCCEEDED(snd_coff_decode_symbol(&coff, sym, &dec));
    CHECK(dec.type == SND_COFF_SYM_TYPE_BSS);
    CHECK_EQ_U32(dec.bss_size, 0x100);

    snd_test_coff_free(&obj);
}

static void test_coff_decode_naked_rejected(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(build_symbol_object(&obj), 0);

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_SUCCEEDED(snd_coff_parse(&buf, &coff));

    PSND_IMAGE_SYMBOL sym = snd_coff_get_symbol_by_index(&coff, 7);
    CHECK(sym != NULL);

    snd_coff_decoded_sym_t dec = {0};
    SND_CHECK_STATUS(snd_coff_decode_symbol(&coff, sym, &dec), SND_STATUS_SYMBOL_NAKED_REJECTED);

    snd_test_coff_free(&obj);
}

static void test_coff_decode_other(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(build_symbol_object(&obj), 0);

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_SUCCEEDED(snd_coff_parse(&buf, &coff));

    PSND_IMAGE_SYMBOL sym = snd_coff_get_symbol_by_index(&coff, 8);
    CHECK(sym != NULL);

    snd_coff_decoded_sym_t dec = {0};
    SND_CHECK_SUCCEEDED(snd_coff_decode_symbol(&coff, sym, &dec));
    CHECK(dec.type == SND_COFF_SYM_TYPE_OTHER);

    snd_test_coff_free(&obj);
}

void snd_test_register_coff_symbols(void) {
    snd_test_register("coff symbols: find short name", test_coff_find_short_name);
    snd_test_register("coff symbols: find long name", test_coff_find_long_name);
    snd_test_register("coff symbols: underscore tolerance", test_coff_find_underscore_tolerance);
    snd_test_register("coff symbols: skips aux records", test_coff_find_skips_aux);
    snd_test_register("coff symbols: decode local", test_coff_decode_local);
    snd_test_register("coff symbols: decode import", test_coff_decode_import);
    snd_test_register("coff symbols: decode bss", test_coff_decode_bss);
    snd_test_register("coff symbols: naked rejected", test_coff_decode_naked_rejected);
    snd_test_register("coff symbols: decode other", test_coff_decode_other);
}

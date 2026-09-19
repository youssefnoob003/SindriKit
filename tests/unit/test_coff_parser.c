#include "coff_builder.h"
#include "test_util.h"

static int build_plain_object(snd_test_coff_t *obj, WORD machine) {
    if (snd_test_coff_build(obj, machine) != 0) {
        return -1;
    }
    return snd_test_coff_finalize(obj);
}

static void test_coff_parse_amd64(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(snd_test_coff_build(&obj, SND_IMAGE_FILE_MACHINE_AMD64), 0);

    CHECK(snd_test_coff_add_section(&obj, ".text", 0x60000020u) == 1);
    CHECK(snd_test_coff_add_section(&obj, ".data", 0xC0000040u) == 2);

    SND_IMAGE_SYMBOL sym = {0};
    memcpy(sym.N.ShortName, "go", 2);
    sym.SectionNumber = 1;
    CHECK(snd_test_coff_add_symbol(&obj, &sym) == 0);

    DWORD long_off = snd_test_coff_add_string(&obj, "very_long_function_name");
    CHECK(long_off != 0);

    SND_IMAGE_SYMBOL sym2 = {0};
    sym2.N.Name.Short     = 0;
    sym2.N.Name.Long      = long_off;
    sym2.SectionNumber    = 1;
    CHECK(snd_test_coff_add_symbol(&obj, &sym2) == 1);

    CHECK_EQ_INT(snd_test_coff_finalize(&obj), 0);

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_SUCCEEDED(snd_coff_parse(&buf, &coff));
    CHECK(coff.is_64bit == TRUE);
    CHECK_EQ_U32(coff.sections_count, 2);
    CHECK(coff.section_head != NULL);
    CHECK(coff.symbol_table != NULL);
    CHECK_EQ_U32(coff.symbol_count, 2);
    CHECK(coff.string_table != NULL);
    CHECK(coff.string_table_size > sizeof(DWORD));

    snd_test_coff_free(&obj);
}

static void test_coff_parse_i386(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(snd_test_coff_build(&obj, SND_IMAGE_FILE_MACHINE_I386), 0);
    CHECK_EQ_INT(snd_test_coff_finalize(&obj), 0);

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_SUCCEEDED(snd_coff_parse(&buf, &coff));
    CHECK(coff.is_64bit == FALSE);

    snd_test_coff_free(&obj);
}

static void test_coff_parse_null_args(void) {
    snd_buffer_t      buf  = {0};
    snd_coff_parser_t coff = {0};
    SND_CHECK_STATUS(snd_coff_parse(NULL, &coff), SND_STATUS_NULL_POINTER);
    SND_CHECK_STATUS(snd_coff_parse(&buf, NULL), SND_STATUS_NULL_POINTER);
}

static void test_coff_parse_truncated_header(void) {
    unsigned char     small[8] = {0};
    snd_buffer_t      buf      = {.data = small, .size = sizeof(small)};
    snd_coff_parser_t coff     = {0};
    SND_CHECK_STATUS(snd_coff_parse(&buf, &coff), SND_STATUS_HEADER_FILE_TRUNCATED);
}

static void test_coff_parse_unsupported_machine(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(snd_test_coff_build(&obj, 0x9000), 0); /* not i386/amd64 */
    CHECK_EQ_INT(snd_test_coff_finalize(&obj), 0);

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_STATUS(snd_coff_parse(&buf, &coff), SND_STATUS_HEADER_MACHINE_UNSUPPORTED);

    snd_test_coff_free(&obj);
}

static void test_coff_parse_truncated_section_table(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(snd_test_coff_build(&obj, SND_IMAGE_FILE_MACHINE_AMD64), 0);
    CHECK(snd_test_coff_add_section(&obj, ".text", 0x20u) == 1);
    CHECK_EQ_INT(snd_test_coff_finalize(&obj), 0);

    /* Cut the buffer so the declared section table is missing. */
    snd_buffer_t      buf  = {.data = snd_test_coff_data(&obj), .size = 20};
    snd_coff_parser_t coff = {0};
    SND_CHECK_STATUS(snd_coff_parse(&buf, &coff), SND_STATUS_SECTION_TABLE_TRUNCATED);

    snd_test_coff_free(&obj);
}

static void test_coff_parse_truncated_symbol_table(void) {
    snd_test_coff_t obj = {0};
    CHECK_EQ_INT(snd_test_coff_build(&obj, SND_IMAGE_FILE_MACHINE_AMD64), 0);

    SND_IMAGE_SYMBOL sym = {0};
    memcpy(sym.N.ShortName, "go", 2);
    sym.SectionNumber = 1;
    CHECK(snd_test_coff_add_symbol(&obj, &sym) == 0);
    CHECK_EQ_INT(snd_test_coff_finalize(&obj), 0);

    /* Point the symbol table past the end of the buffer. */
    PSND_IMAGE_FILE_HEADER fh = (PSND_IMAGE_FILE_HEADER)snd_test_coff_data(&obj);
    fh->PointerToSymbolTable  = (DWORD)snd_test_coff_size(&obj) + 0x100;

    snd_buffer_t      buf  = snd_test_coff_as_buffer(&obj);
    snd_coff_parser_t coff = {0};
    SND_CHECK_STATUS(snd_coff_parse(&buf, &coff), SND_STATUS_SYMBOL_TABLE_TRUNCATED);

    snd_test_coff_free(&obj);
}

void snd_test_register_coff_parser(void) {
    snd_test_register("coff parser: AMD64 object", test_coff_parse_amd64);
    snd_test_register("coff parser: i386 object", test_coff_parse_i386);
    snd_test_register("coff parser: null args", test_coff_parse_null_args);
    snd_test_register("coff parser: truncated header", test_coff_parse_truncated_header);
    snd_test_register("coff parser: unsupported machine", test_coff_parse_unsupported_machine);
    snd_test_register("coff parser: truncated section table", test_coff_parse_truncated_section_table);
    snd_test_register("coff parser: truncated symbol table", test_coff_parse_truncated_symbol_table);
}
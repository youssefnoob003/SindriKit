#include "pe_builder.h"
#include "test_util.h"

/* Block A: DIR64 at 0x70 + ABSOLUTE padding; Block B: HIGHLOW at 0x20. */
#define RELOC_VA_A 0x1000u
#define RELOC_VA_B 0x2000u

#define RELOC_ENTRIES_A    2u
#define RELOC_ENTRIES_B    1u
#define RELOC_BLOCK_A_SIZE (sizeof(SND_IMAGE_BASE_RELOCATION) + 2u * RELOC_ENTRIES_A)
#define RELOC_BLOCK_B_SIZE (sizeof(SND_IMAGE_BASE_RELOCATION) + 2u * RELOC_ENTRIES_B)

static int build_image(snd_test_pe_t *img, const snd_test_pe_spec_t *spec) {
    if (snd_test_pe_build(img, spec) != 0) {
        return -1;
    }
    return snd_test_pe_finalize(img);
}

static void build_reloc_blob(unsigned char *p, size_t cap) {
    memset(p, 0, cap);

    snd_test_put32(p, 0, RELOC_VA_A);
    snd_test_put32(p, 4, (DWORD)RELOC_BLOCK_A_SIZE);
    snd_test_put16(p, 8, (WORD)((SND_IMAGE_REL_BASED_DIR64 << 12) | 0x70u));
    snd_test_put16(p, 10, (WORD)((SND_IMAGE_REL_BASED_ABSOLUTE << 12) | 0x00u));

    snd_test_put32(p, RELOC_BLOCK_A_SIZE, RELOC_VA_B);
    snd_test_put32(p, RELOC_BLOCK_A_SIZE + 4, (DWORD)RELOC_BLOCK_B_SIZE);
    snd_test_put16(p, RELOC_BLOCK_A_SIZE + 8, (WORD)((SND_IMAGE_REL_BASED_HIGHLOW << 12) | 0x20u));
}

static int build_reloc_image(snd_test_pe_t *img, snd_test_pe_region_t *relocs) {
    snd_test_pe_spec_t spec = {.is_64bit = 1};
    if (snd_test_pe_build(img, &spec) != 0) {
        return -1;
    }

    /* Extend the section so patch targets at 0x2020 stay mapped. */
    unsigned char pad[0x1100] = {0};
    (void)snd_test_pe_add_region(img, pad, sizeof(pad));

    unsigned char blob[RELOC_BLOCK_A_SIZE + RELOC_BLOCK_B_SIZE];
    build_reloc_blob(blob, sizeof(blob));
    *relocs = snd_test_pe_add_region(img, blob, sizeof(blob));

    snd_test_pe_set_directory(img, SND_IMAGE_DIRECTORY_ENTRY_BASERELOC, relocs);
    return snd_test_pe_finalize(img);
}

static void test_reloc_block_walk(void) {
    snd_test_pe_t        img    = {0};
    snd_test_pe_region_t relocs = {0};
    CHECK_EQ_INT(build_reloc_image(&img, &relocs), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    SIZE_T                           cursor = 0;
    const SND_IMAGE_BASE_RELOCATION *block  = NULL;
    DWORD                            count  = 0;

    SND_CHECK_SUCCEEDED(snd_pe_get_reloc_block(&pe, &cursor, &block, &count));
    CHECK(block != NULL);
    CHECK_EQ_U32(block->VirtualAddress, RELOC_VA_A);
    CHECK_EQ_U32(count, RELOC_ENTRIES_A);

    SND_CHECK_SUCCEEDED(snd_pe_get_reloc_block(&pe, &cursor, &block, &count));
    CHECK(block != NULL);
    CHECK_EQ_U32(block->VirtualAddress, RELOC_VA_B);
    CHECK_EQ_U32(count, RELOC_ENTRIES_B);

    SND_CHECK_SUCCEEDED(snd_pe_get_reloc_block(&pe, &cursor, &block, &count));
    CHECK(block == NULL);

    snd_test_pe_free(&img);
}

static void test_reloc_entries(void) {
    snd_test_pe_t        img    = {0};
    snd_test_pe_region_t relocs = {0};
    CHECK_EQ_INT(build_reloc_image(&img, &relocs), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    SIZE_T                           cursor = 0;
    const SND_IMAGE_BASE_RELOCATION *block  = NULL;
    DWORD                            count  = 0;
    SND_CHECK_SUCCEEDED(snd_pe_get_reloc_block(&pe, &cursor, &block, &count));

    snd_pe_reloc_entry_t entry = {0};
    SND_CHECK_SUCCEEDED(snd_pe_get_reloc_entry(&pe, block, 0, &entry));
    CHECK_EQ_U32(entry.type, SND_IMAGE_REL_BASED_DIR64);
    CHECK_EQ_U32(entry.patch_rva, RELOC_VA_A + 0x70);
    CHECK(entry.patch_ptr != NULL);

    SND_CHECK_SUCCEEDED(snd_pe_get_reloc_entry(&pe, block, 1, &entry));
    CHECK_EQ_U32(entry.type, SND_IMAGE_REL_BASED_ABSOLUTE);
    CHECK(entry.patch_ptr == NULL);

    SND_CHECK_STATUS(snd_pe_get_reloc_entry(&pe, block, 5, &entry), SND_STATUS_RELOCATION_ENTRY_OVERFLOW);

    /* Second block's HIGHLOW. */
    SND_CHECK_SUCCEEDED(snd_pe_get_reloc_block(&pe, &cursor, &block, &count));
    SND_CHECK_SUCCEEDED(snd_pe_get_reloc_entry(&pe, block, 0, &entry));
    CHECK_EQ_U32(entry.type, SND_IMAGE_REL_BASED_HIGHLOW);
    CHECK_EQ_U32(entry.patch_rva, RELOC_VA_B + 0x20);
    CHECK(entry.patch_ptr != NULL);

    snd_test_pe_free(&img);
}

static void test_reloc_unsupported_type(void) {
    snd_test_pe_t        img    = {0};
    snd_test_pe_region_t relocs = {0};
    CHECK_EQ_INT(build_reloc_image(&img, &relocs), 0);

    /* Rewrite the first entry with an unsupported relocation type. */
    size_t off = (size_t)(relocs.rva - SND_TEST_PE_SEC_BASE) + sizeof(SND_IMAGE_BASE_RELOCATION);
    snd_test_put16(snd_test_pe_data(&img), img.section_off + off, (WORD)((7 << 12) | 0x70u));

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    SIZE_T                           cursor = 0;
    const SND_IMAGE_BASE_RELOCATION *block  = NULL;
    DWORD                            count  = 0;
    SND_CHECK_SUCCEEDED(snd_pe_get_reloc_block(&pe, &cursor, &block, &count));

    snd_pe_reloc_entry_t entry = {0};
    SND_CHECK_STATUS(snd_pe_get_reloc_entry(&pe, block, 0, &entry), SND_STATUS_PE_RELOCATION_TYPE_UNSUPPORTED);

    snd_test_pe_free(&img);
}

static void test_reloc_invalid_block_size(void) {
    snd_test_pe_t        img    = {0};
    snd_test_pe_region_t relocs = {0};
    CHECK_EQ_INT(build_reloc_image(&img, &relocs), 0);

    /* First block's SizeOfBlock = 0xFFFFFFFF. */
    size_t off = (size_t)(relocs.rva - SND_TEST_PE_SEC_BASE) + sizeof(DWORD);
    snd_test_put32(snd_test_pe_data(&img), img.section_off + off, 0xFFFFFFFFu);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    SIZE_T                           cursor = 0;
    const SND_IMAGE_BASE_RELOCATION *block  = NULL;
    DWORD                            count  = 0;
    SND_CHECK_STATUS(snd_pe_get_reloc_block(&pe, &cursor, &block, &count), SND_STATUS_RELOCATION_BLOCK_INVALID);

    snd_test_pe_free(&img);
}

static void test_reloc_directory_truncated(void) {
    snd_test_pe_spec_t spec = {.is_64bit = 1};
    snd_test_pe_t      img  = {0};
    CHECK_EQ_INT(build_image(&img, &spec), 0);

    /* Directory size smaller than a block header. */
    snd_test_pe_set_directory_rva(&img, SND_IMAGE_DIRECTORY_ENTRY_BASERELOC, SND_TEST_PE_SEC_BASE, 4);
    CHECK_EQ_INT(snd_test_pe_finalize(&img), 0);

    snd_buffer_t    buf = snd_test_pe_as_buffer(&img);
    snd_pe_parser_t pe  = {0};
    SND_CHECK_SUCCEEDED(snd_pe_parse(&buf, FALSE, &pe));

    SIZE_T                           cursor = 0;
    const SND_IMAGE_BASE_RELOCATION *block  = NULL;
    DWORD                            count  = 0;
    SND_CHECK_STATUS(snd_pe_get_reloc_block(&pe, &cursor, &block, &count), SND_STATUS_RELOCATION_DIRECTORY_TRUNCATED);

    snd_test_pe_free(&img);
}

void snd_test_register_pe_relocs(void) {
    snd_test_register("pe relocs: block walk", test_reloc_block_walk);
    snd_test_register("pe relocs: entry decode", test_reloc_entries);
    snd_test_register("pe relocs: unsupported type", test_reloc_unsupported_type);
    snd_test_register("pe relocs: invalid block size", test_reloc_invalid_block_size);
    snd_test_register("pe relocs: truncated directory", test_reloc_directory_truncated);
}
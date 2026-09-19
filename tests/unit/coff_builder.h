#ifndef SND_TEST_COFF_BUILDER_H
#define SND_TEST_COFF_BUILDER_H

#include <sindri/common/buffer.h>
#include <sindri/internal/windows/coff.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char *data;
    size_t         size;
    size_t         cap;
} snd_test_coff_mem_t;

typedef struct {
    snd_test_coff_mem_t mem;
    WORD                machine;
    DWORD               sections_count;
    DWORD               symbol_table_off; /* patched on finalize       */
    DWORD               symbol_count;
    SND_IMAGE_SYMBOL   *symbols;
    size_t              symbol_capacity;
    unsigned char      *strings; /* contents after the size DWORD */
    size_t              string_capacity;
    size_t              string_count;
} snd_test_coff_t;

/* Allocates and initializes the 20-byte COFF file header. Returns 0 on
 * success. `machine` is SND_IMAGE_FILE_MACHINE_AMD64 or ..._I386. */
int snd_test_coff_build(snd_test_coff_t *obj, WORD machine);

/* Appends one section header. Returns its 1-based section number or -1. */
int snd_test_coff_add_section(snd_test_coff_t *obj, const char name[8], DWORD characteristics);

/* Appends one packed symbol. Returns its symbol-table index or -1. */
int snd_test_coff_add_symbol(snd_test_coff_t *obj, const SND_IMAGE_SYMBOL *sym);

/* Appends a NUL-terminated string to the string table. Returns its offset
 * within the table (>= 4, i.e. pointing past the size DWORD) or 0 on failure. */
DWORD snd_test_coff_add_string(snd_test_coff_t *obj, const char *str);

/* Patches NumberOfSections / PointerToSymbolTable / NumberOfSymbols and the
 * string-table size DWORD. Returns 0 on success. */
int snd_test_coff_finalize(snd_test_coff_t *obj);

/* Adopts the object for a parser; the parser will NOT free this buffer. */
snd_buffer_t snd_test_coff_as_buffer(snd_test_coff_t *obj);

unsigned char *snd_test_coff_data(snd_test_coff_t *obj);
size_t         snd_test_coff_size(snd_test_coff_t *obj);

void snd_test_coff_free(snd_test_coff_t *obj);

#ifdef __cplusplus
}
#endif

#endif /* SND_TEST_COFF_BUILDER_H */

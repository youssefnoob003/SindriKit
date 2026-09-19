#include "coff_builder.h"

#include <stdlib.h>
#include <string.h>

static int snd_test_coff_mem_reserve(snd_test_coff_mem_t *m, size_t extra) {
    if (extra <= m->cap) {
        return 0;
    }
    size_t ncap = m->cap ? m->cap : 256;
    while (ncap < extra) {
        ncap *= 2;
    }
    unsigned char *nd = (unsigned char *)realloc(m->data, ncap);
    if (nd == NULL) {
        return -1;
    }
    memset(nd + m->cap, 0, ncap - m->cap);
    m->data = nd;
    m->cap  = ncap;
    return 0;
}

static int snd_test_coff_symbols_reserve(snd_test_coff_t *obj, size_t count) {
    if (count <= obj->symbol_capacity) {
        return 0;
    }
    size_t capacity = obj->symbol_capacity ? obj->symbol_capacity : 16;
    while (capacity < count) {
        capacity *= 2;
    }
    SND_IMAGE_SYMBOL *symbols = (SND_IMAGE_SYMBOL *)realloc(obj->symbols, capacity * sizeof(*symbols));
    if (symbols == NULL) {
        return -1;
    }
    obj->symbols         = symbols;
    obj->symbol_capacity = capacity;
    return 0;
}

static int snd_test_coff_strings_reserve(snd_test_coff_t *obj, size_t size) {
    if (size <= obj->string_capacity) {
        return 0;
    }
    size_t capacity = obj->string_capacity ? obj->string_capacity : 64;
    while (capacity < size) {
        capacity *= 2;
    }
    unsigned char *strings = (unsigned char *)realloc(obj->strings, capacity);
    if (strings == NULL) {
        return -1;
    }
    obj->strings         = strings;
    obj->string_capacity = capacity;
    return 0;
}

static void snd_test_put32_lcl(unsigned char *p, size_t off, DWORD v) {
    p[off]     = (unsigned char)(v & 0xFF);
    p[off + 1] = (unsigned char)((v >> 8) & 0xFF);
    p[off + 2] = (unsigned char)((v >> 16) & 0xFF);
    p[off + 3] = (unsigned char)((v >> 24) & 0xFF);
}

static int snd_test_coff_mem_size(snd_test_coff_mem_t *m, size_t size) {
    if (snd_test_coff_mem_reserve(m, size) != 0) {
        return -1;
    }
    if (size > m->size) {
        memset(m->data + m->size, 0, size - m->size);
        m->size = size;
    }
    return 0;
}

int snd_test_coff_build(snd_test_coff_t *obj, WORD machine) {
    if (obj == NULL) {
        return -1;
    }
    memset(obj, 0, sizeof(*obj));
    obj->machine = machine;

    if (snd_test_coff_mem_size(&obj->mem, sizeof(SND_IMAGE_FILE_HEADER)) != 0) {
        return -1;
    }

    PSND_IMAGE_FILE_HEADER fh = (PSND_IMAGE_FILE_HEADER)obj->mem.data;
    fh->Machine               = machine;
    fh->SizeOfOptionalHeader  = 0;
    return 0;
}

int snd_test_coff_add_section(snd_test_coff_t *obj, const char name[8], DWORD characteristics) {
    if (obj == NULL || name == NULL) {
        return -1;
    }
    size_t off = sizeof(SND_IMAGE_FILE_HEADER) + (size_t)obj->sections_count * sizeof(SND_IMAGE_SECTION_HEADER);
    if (snd_test_coff_mem_size(&obj->mem, off + sizeof(SND_IMAGE_SECTION_HEADER)) != 0) {
        return -1;
    }

    PSND_IMAGE_SECTION_HEADER sec = (PSND_IMAGE_SECTION_HEADER)(obj->mem.data + off);
    memset(sec, 0, sizeof(*sec));
    memcpy(sec->Name, name, 8);
    sec->Characteristics = characteristics;

    obj->sections_count++;
    return (int)obj->sections_count;
}

int snd_test_coff_add_symbol(snd_test_coff_t *obj, const SND_IMAGE_SYMBOL *sym) {
    if (obj == NULL || sym == NULL) {
        return -1;
    }

    if (snd_test_coff_symbols_reserve(obj, (size_t)obj->symbol_count + 1) != 0) {
        return -1;
    }
    obj->symbols[obj->symbol_count] = *sym;

    obj->symbol_count++;
    return (int)obj->symbol_count - 1;
}

DWORD snd_test_coff_add_string(snd_test_coff_t *obj, const char *str) {
    if (obj == NULL || str == NULL) {
        return 0;
    }

    size_t len = strlen(str) + 1; /* include the NUL terminator */
    size_t off = obj->string_count;
    if (snd_test_coff_strings_reserve(obj, off + len) != 0) {
        return 0;
    }
    memcpy(obj->strings + off, str, len);

    obj->string_count += len;
    return (DWORD)(sizeof(DWORD) + off);
}

int snd_test_coff_finalize(snd_test_coff_t *obj) {
    if (obj == NULL || obj->mem.data == NULL) {
        return -1;
    }

    size_t header_size = sizeof(SND_IMAGE_FILE_HEADER) + (size_t)obj->sections_count * sizeof(SND_IMAGE_SECTION_HEADER);
    if (snd_test_coff_mem_size(&obj->mem, header_size) != 0) {
        return -1;
    }

    size_t cursor         = header_size;
    obj->symbol_table_off = obj->symbol_count != 0 ? (DWORD)cursor : 0;
    if (obj->symbol_count != 0) {
        size_t symbol_bytes = (size_t)obj->symbol_count * sizeof(SND_IMAGE_SYMBOL);
        if (snd_test_coff_mem_size(&obj->mem, cursor + symbol_bytes) != 0) {
            return -1;
        }
        memcpy(obj->mem.data + cursor, obj->symbols, symbol_bytes);
        cursor += symbol_bytes;
    }

    if (obj->string_count != 0) {
        size_t string_bytes = sizeof(DWORD) + obj->string_count;
        if (snd_test_coff_mem_size(&obj->mem, cursor + string_bytes) != 0) {
            return -1;
        }
        snd_test_put32_lcl(obj->mem.data, cursor, (DWORD)string_bytes);
        memcpy(obj->mem.data + cursor + sizeof(DWORD), obj->strings, obj->string_count);
        cursor += string_bytes;
    }

    PSND_IMAGE_FILE_HEADER fh = (PSND_IMAGE_FILE_HEADER)obj->mem.data;
    fh->NumberOfSections      = (WORD)obj->sections_count;
    fh->PointerToSymbolTable  = obj->symbol_table_off;
    fh->NumberOfSymbols       = obj->symbol_count;
    return 0;
}

snd_buffer_t snd_test_coff_as_buffer(snd_test_coff_t *obj) {
    snd_buffer_t buf = {0};
    if (obj == NULL) {
        return buf;
    }
    buf.data = obj->mem.data;
    buf.size = obj->mem.size;
    return buf;
}

unsigned char *snd_test_coff_data(snd_test_coff_t *obj) {
    return obj != NULL ? obj->mem.data : NULL;
}

size_t snd_test_coff_size(snd_test_coff_t *obj) {
    return obj != NULL ? obj->mem.size : 0;
}

void snd_test_coff_free(snd_test_coff_t *obj) {
    if (obj == NULL) {
        return;
    }
    free(obj->mem.data);
    free(obj->symbols);
    free(obj->strings);
    memset(obj, 0, sizeof(*obj));
}

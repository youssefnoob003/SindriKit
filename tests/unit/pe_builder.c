#include "pe_builder.h"

#include <stdlib.h>
#include <string.h>

#define SND_IMAGE_FILE_MACHINE_AMD64_LOCAL 0x8664u
#define SND_IMAGE_FILE_MACHINE_I386_LOCAL  0x014cu

static size_t snd_test_align_up(size_t v, size_t a) {
    return (v + (a - 1)) & ~(a - 1);
}

static int snd_test_mem_reserve(snd_test_mem_t *m, size_t extra) {
    if (extra <= m->cap) {
        return 0;
    }
    size_t         ncap = m->cap ? m->cap : 256;
    unsigned char *nd   = NULL;
    while (ncap < extra) {
        ncap *= 2;
    }
    nd = (unsigned char *)realloc(m->data, ncap);
    if (nd == NULL) {
        return -1;
    }
    memset(nd + m->cap, 0, ncap - m->cap);
    m->data = nd;
    m->cap  = ncap;
    return 0;
}

static int snd_test_mem_size(snd_test_mem_t *m, size_t size) {
    if (snd_test_mem_reserve(m, size) != 0) {
        return -1;
    }
    if (size > m->size) {
        memset(m->data + m->size, 0, size - m->size);
        m->size = size;
    }
    return 0;
}

void snd_test_put16(unsigned char *p, size_t off, WORD v) {
    p[off]     = (unsigned char)(v & 0xFF);
    p[off + 1] = (unsigned char)((v >> 8) & 0xFF);
}

void snd_test_put32(unsigned char *p, size_t off, DWORD v) {
    p[off]     = (unsigned char)(v & 0xFF);
    p[off + 1] = (unsigned char)((v >> 8) & 0xFF);
    p[off + 2] = (unsigned char)((v >> 16) & 0xFF);
    p[off + 3] = (unsigned char)((v >> 24) & 0xFF);
}

void snd_test_put64(unsigned char *p, size_t off, ULONGLONG v) {
    for (int i = 0; i < 8; i++) {
        p[off + i] = (unsigned char)((v >> (8 * i)) & 0xFF);
    }
}

int snd_test_pe_build(snd_test_pe_t *img, const snd_test_pe_spec_t *spec) {
    if (img == NULL || spec == NULL) {
        return -1;
    }

    memset(img, 0, sizeof(*img));
    img->spec        = *spec;
    img->section_off = spec->is_mapped ? SND_TEST_PE_SEC_BASE : SND_TEST_PE_HDR_SIZE;

    if (snd_test_mem_size(&img->mem, img->section_off) != 0) {
        return -1;
    }

    unsigned char *p = img->mem.data;

    /* DOS header: only e_magic and e_lfanew matter to the parser. */
    PSND_IMAGE_DOS_HEADER dos = (PSND_IMAGE_DOS_HEADER)p;
    dos->e_magic              = SND_IMAGE_DOS_SIGNATURE;
    dos->e_lfanew             = SND_TEST_PE_LFANEW;

    /* NT signature. */
    DWORD *sig = (DWORD *)(p + SND_TEST_PE_LFANEW);
    *sig       = SND_IMAGE_NT_SIGNATURE;

    /* File header. */
    PSND_IMAGE_FILE_HEADER fh = (PSND_IMAGE_FILE_HEADER)(p + SND_TEST_PE_LFANEW + sizeof(DWORD));
    fh->Machine               = spec->is_64bit ? SND_IMAGE_FILE_MACHINE_AMD64_LOCAL : SND_IMAGE_FILE_MACHINE_I386_LOCAL;
    fh->NumberOfSections      = 1;
    fh->Characteristics       = spec->is_dll ? SND_IMAGE_FILE_DLL : 0;
    fh->SizeOfOptionalHeader =
        (WORD)(spec->is_64bit ? sizeof(SND_IMAGE_OPTIONAL_HEADER64) : sizeof(SND_IMAGE_OPTIONAL_HEADER32));

    /* Optional header. */
    unsigned char *opt = (unsigned char *)fh + sizeof(SND_IMAGE_FILE_HEADER);
    if (spec->is_64bit) {
        PSND_IMAGE_OPTIONAL_HEADER64 o = (PSND_IMAGE_OPTIONAL_HEADER64)opt;
        o->Magic                       = SND_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
        o->SectionAlignment            = SND_TEST_PE_FILE_ALIGN;
        o->FileAlignment               = SND_TEST_PE_FILE_ALIGN;
        o->SizeOfHeaders               = SND_TEST_PE_HDR_SIZE;
        o->SizeOfImage                 = SND_TEST_PE_SEC_BASE;
        o->NumberOfRvaAndSizes         = SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
        o->Subsystem                   = 3;
        o->ImageBase                   = 0x140000000ull;
    } else {
        PSND_IMAGE_OPTIONAL_HEADER32 o = (PSND_IMAGE_OPTIONAL_HEADER32)opt;
        o->Magic                       = SND_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
        o->SectionAlignment            = SND_TEST_PE_FILE_ALIGN;
        o->FileAlignment               = SND_TEST_PE_FILE_ALIGN;
        o->SizeOfHeaders               = SND_TEST_PE_HDR_SIZE;
        o->SizeOfImage                 = SND_TEST_PE_SEC_BASE;
        o->NumberOfRvaAndSizes         = SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
        o->Subsystem                   = 3;
        o->ImageBase                   = 0x400000;
    }

    return 0;
}

snd_test_pe_region_t snd_test_pe_add_region(snd_test_pe_t *img, const void *bytes, size_t len) {
    snd_test_pe_region_t r = {0, 0};
    if (img == NULL || (len != 0 && bytes == NULL)) {
        return r;
    }

    size_t off = img->section_off + img->section_len;
    if (snd_test_mem_size(&img->mem, off + len) != 0) {
        return r;
    }
    if (len != 0) {
        memcpy(img->mem.data + off, bytes, len);
    }

    r.rva  = SND_TEST_PE_SEC_BASE + (DWORD)img->section_len;
    r.size = (DWORD)len;
    img->section_len += len;
    return r;
}

void snd_test_pe_set_directory(snd_test_pe_t *img, int index, const snd_test_pe_region_t *region) {
    if (img == NULL || region == NULL || index < 0 || index >= SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES) {
        return;
    }
    img->dir_rva[index]  = region->rva;
    img->dir_size[index] = region->size;
}

void snd_test_pe_set_directory_rva(snd_test_pe_t *img, int index, DWORD rva, DWORD size) {
    if (img == NULL || index < 0 || index >= SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES) {
        return;
    }
    img->dir_rva[index]  = rva;
    img->dir_size[index] = size;
}

int snd_test_pe_finalize(snd_test_pe_t *img) {
    if (img == NULL || img->mem.data == NULL) {
        return -1;
    }

    size_t aligned_len = snd_test_align_up(img->section_len, SND_TEST_PE_FILE_ALIGN);
    DWORD  size_of_image =
        (DWORD)snd_test_align_up((size_t)SND_TEST_PE_SEC_BASE + img->section_len, SND_TEST_PE_FILE_ALIGN);

    /* Total image byte length differs by layout: raw file layout stops after
     * the section data; the mapped layout must cover the declared SizeOfImage. */
    size_t want = img->spec.is_mapped ? (size_t)size_of_image : (size_t)img->section_off + aligned_len;
    if (want > img->mem.size) {
        if (snd_test_mem_size(&img->mem, want) != 0) {
            return -1;
        }
    }

    unsigned char *p = img->mem.data;

    size_t sec_off = SND_TEST_PE_LFANEW + sizeof(DWORD) + sizeof(SND_IMAGE_FILE_HEADER) +
                     (img->spec.is_64bit ? sizeof(SND_IMAGE_OPTIONAL_HEADER64) : sizeof(SND_IMAGE_OPTIONAL_HEADER32));

    PSND_IMAGE_SECTION_HEADER sec = (PSND_IMAGE_SECTION_HEADER)(p + sec_off);
    memset(sec, 0, sizeof(*sec));
    memcpy(sec->Name, ".text", 5);
    sec->Misc.VirtualSize = (DWORD)img->section_len;
    sec->VirtualAddress   = SND_TEST_PE_SEC_BASE;
    sec->SizeOfRawData    = (DWORD)aligned_len;
    sec->PointerToRawData = img->section_off;
    sec->Characteristics  = 0x40000000u; /* IMAGE_SCN_MEM_READ */

    if (img->spec.is_64bit) {
        PSND_IMAGE_OPTIONAL_HEADER64 o =
            (PSND_IMAGE_OPTIONAL_HEADER64)(p + SND_TEST_PE_LFANEW + sizeof(DWORD) + sizeof(SND_IMAGE_FILE_HEADER));
        o->SizeOfImage         = size_of_image;
        o->AddressOfEntryPoint = img->spec.entry_point_rva;
        for (int i = 0; i < SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++) {
            o->DataDirectory[i].VirtualAddress = img->dir_rva[i];
            o->DataDirectory[i].Size           = img->dir_size[i];
        }
    } else {
        PSND_IMAGE_OPTIONAL_HEADER32 o =
            (PSND_IMAGE_OPTIONAL_HEADER32)(p + SND_TEST_PE_LFANEW + sizeof(DWORD) + sizeof(SND_IMAGE_FILE_HEADER));
        o->SizeOfImage         = size_of_image;
        o->AddressOfEntryPoint = img->spec.entry_point_rva;
        for (int i = 0; i < SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++) {
            o->DataDirectory[i].VirtualAddress = img->dir_rva[i];
            o->DataDirectory[i].Size           = img->dir_size[i];
        }
    }

    return 0;
}

snd_buffer_t snd_test_pe_as_buffer(snd_test_pe_t *img) {
    snd_buffer_t buf = {0};
    if (img == NULL) {
        return buf;
    }
    buf.data = img->mem.data;
    buf.size = img->mem.size;
    return buf;
}

unsigned char *snd_test_pe_data(snd_test_pe_t *img) {
    return img != NULL ? img->mem.data : NULL;
}

size_t snd_test_pe_size(snd_test_pe_t *img) {
    return img != NULL ? img->mem.size : 0;
}

void snd_test_pe_free(snd_test_pe_t *img) {
    if (img == NULL) {
        return;
    }
    free(img->mem.data);
    memset(img, 0, sizeof(*img));
}
#ifndef SND_INTERNAL_WINDOWS_COFF_H
#define SND_INTERNAL_WINDOWS_COFF_H

#include <sindri/internal/windows/image.h>

/*
 * COFF object file types that go beyond basic PE structures.
 * SND_IMAGE_FILE_HEADER and SND_IMAGE_SECTION_HEADER are shared with PE
 * and live in windows/image.h.
 */

/* Machine types */
#define SND_IMAGE_FILE_MACHINE_I386  0x014c
#define SND_IMAGE_FILE_MACHINE_AMD64 0x8664

/* Symbol constants */
#define SND_IMAGE_SYM_UNDEFINED 0

/* Section flags used by COFF relocations */
#define SND_IMAGE_SCN_LNK_NRELOC_OVFL 0x01000000

/* AMD64 relocation types */
#define SND_IMAGE_REL_AMD64_ABSOLUTE 0x0000
#define SND_IMAGE_REL_AMD64_ADDR64   0x0001
#define SND_IMAGE_REL_AMD64_ADDR32   0x0002
#define SND_IMAGE_REL_AMD64_ADDR32NB 0x0003
#define SND_IMAGE_REL_AMD64_REL32    0x0004
#define SND_IMAGE_REL_AMD64_REL32_1  0x0005
#define SND_IMAGE_REL_AMD64_REL32_2  0x0006
#define SND_IMAGE_REL_AMD64_REL32_3  0x0007
#define SND_IMAGE_REL_AMD64_REL32_4  0x0008
#define SND_IMAGE_REL_AMD64_REL32_5  0x0009

/* i386 relocation types */
#define SND_IMAGE_REL_I386_ABSOLUTE 0x0000
#define SND_IMAGE_REL_I386_DIR16    0x0001
#define SND_IMAGE_REL_I386_REL16    0x0002
#define SND_IMAGE_REL_I386_DIR32    0x0006
#define SND_IMAGE_REL_I386_DIR32NB  0x0007
#define SND_IMAGE_REL_I386_REL32    0x0014

#pragma pack(push, 1)

/* COFF Symbol Table Entry */
typedef struct _SND_IMAGE_SYMBOL {
    union {
        BYTE ShortName[8];
        struct {
            DWORD Short;
            DWORD Long;
        } Name;
        DWORD LongName[2];
    } N;
    DWORD Value;
    SHORT SectionNumber;
    WORD  Type;
    BYTE  StorageClass;
    BYTE  NumberOfAuxSymbols;
} SND_IMAGE_SYMBOL, *PSND_IMAGE_SYMBOL;

/* COFF Relocation Entry */
typedef struct _SND_IMAGE_RELOCATION {
    union {
        DWORD VirtualAddress;
        DWORD RelocCount;
    } u;
    DWORD SymbolTableIndex;
    WORD  Type;
} SND_IMAGE_RELOCATION, *PSND_IMAGE_RELOCATION;

#pragma pack(pop)

typedef char snd_image_symbol_size_must_be_18[(sizeof(SND_IMAGE_SYMBOL) == SND_IMAGE_SIZEOF_SYMBOL) ? 1 : -1];
typedef char snd_image_relocation_size_must_be_10[(sizeof(SND_IMAGE_RELOCATION) == 10) ? 1 : -1];

#endif /* SND_INTERNAL_WINDOWS_COFF_H */

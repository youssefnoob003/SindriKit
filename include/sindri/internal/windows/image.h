#ifndef SND_INTERNAL_WINDOWS_IMAGE_H
#define SND_INTERNAL_WINDOWS_IMAGE_H

#include <sindri/internal/windows/types.h>

#define SND_IMAGE_DOS_SIGNATURE     0x5A4D
#define SND_IMAGE_NT_SIGNATURE      0x00004550
#define SND_IMAGE_SIZEOF_SHORT_NAME 8
#define SND_IMAGE_SIZEOF_SYMBOL     18

#pragma pack(push, 1)
typedef struct _SND_IMAGE_DOS_HEADER {
    WORD e_magic;
    WORD e_cblp;
    WORD e_cp;
    WORD e_crlc;
    WORD e_cparhdr;
    WORD e_minalloc;
    WORD e_maxalloc;
    WORD e_ss;
    WORD e_sp;
    WORD e_csum;
    WORD e_ip;
    WORD e_cs;
    WORD e_lfarlc;
    WORD e_ovno;
    WORD e_res[4];
    WORD e_oemid;
    WORD e_oeminfo;
    WORD e_res2[10];
    LONG e_lfanew;
} SND_IMAGE_DOS_HEADER, *PSND_IMAGE_DOS_HEADER;
#pragma pack(pop)

typedef struct _SND_IMAGE_FILE_HEADER {
    WORD  Machine;
    WORD  NumberOfSections;
    DWORD TimeDateStamp;
    DWORD PointerToSymbolTable;
    DWORD NumberOfSymbols;
    WORD  SizeOfOptionalHeader;
    WORD  Characteristics;
} SND_IMAGE_FILE_HEADER, *PSND_IMAGE_FILE_HEADER;

typedef struct _SND_IMAGE_DATA_DIRECTORY {
    DWORD VirtualAddress;
    DWORD Size;
} SND_IMAGE_DATA_DIRECTORY, *PSND_IMAGE_DATA_DIRECTORY;

typedef struct _SND_IMAGE_SECTION_HEADER {
    BYTE Name[SND_IMAGE_SIZEOF_SHORT_NAME];
    union {
        DWORD PhysicalAddress;
        DWORD VirtualSize;
    } Misc;
    DWORD VirtualAddress;
    DWORD SizeOfRawData;
    DWORD PointerToRawData;
    DWORD PointerToRelocations;
    DWORD PointerToLinenumbers;
    WORD  NumberOfRelocations;
    WORD  NumberOfLinenumbers;
    DWORD Characteristics;
} SND_IMAGE_SECTION_HEADER, *PSND_IMAGE_SECTION_HEADER;

#define SND_IMAGE_SCN_MEM_EXECUTE 0x20000000
#define SND_IMAGE_SCN_MEM_READ    0x40000000
#define SND_IMAGE_SCN_MEM_WRITE   0x80000000

#endif /* SND_INTERNAL_WINDOWS_IMAGE_H */

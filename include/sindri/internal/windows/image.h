#ifndef SND_INTERNAL_WINDOWS_IMAGE_H
#define SND_INTERNAL_WINDOWS_IMAGE_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/constants.h>
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

SND_BEGIN_EXTERN_C

/**
 * @brief Maps PE/COFF section memory characteristic flags to `SND_PAGE_*` protection flags.
 * @param characteristics Combination of `SND_IMAGE_SCN_MEM_*` flags.
 * @retval Equivalent `SND_PAGE_*` constant (`SND_PAGE_NOACCESS` if none are set).
 */
SND_FORCE_INLINE DWORD snd_image_characteristics_to_protect(DWORD characteristics) {
    static const DWORD protect_lookup[8] = {
        SND_PAGE_NOACCESS,          // 000: No access flags set
        SND_PAGE_READWRITE,         // 001: Write-only (Windows elevates to RW)
        SND_PAGE_READONLY,          // 010: Read-only
        SND_PAGE_READWRITE,         // 011: Read/Write
        SND_PAGE_EXECUTE,           // 100: Execute-only
        SND_PAGE_EXECUTE_READWRITE, // 101: Execute/Write (Windows elevates to ERW)
        SND_PAGE_EXECUTE_READ,      // 110: Execute/Read
        SND_PAGE_EXECUTE_READWRITE  // 111: Execute/Read/Write
    };

    DWORD index = ((characteristics & SND_IMAGE_SCN_MEM_EXECUTE) ? 4 : 0) |
                  ((characteristics & SND_IMAGE_SCN_MEM_READ) ? 2 : 0) |
                  ((characteristics & SND_IMAGE_SCN_MEM_WRITE) ? 1 : 0);

    return protect_lookup[index];
}

SND_END_EXTERN_C

#endif /* SND_INTERNAL_WINDOWS_IMAGE_H */

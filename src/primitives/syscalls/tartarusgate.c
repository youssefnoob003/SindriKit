#include "internal/neighbor.h"

#include <sindri/common/helpers.h>
#include <sindri/common/status.h>
#include <sindri/parsers/pe/pe_exports.h>
#include <sindri/parsers/pe/pe_parser.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>
#include <windows.h>

snd_status_t snd_tartarus_extract_syscall(PVOID ntdll_base, DWORD func_hash, snd_syscall_entry_t *entry_out) {
    if (!entry_out || !ntdll_base) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    FARPROC      func_addr = NULL;
    snd_status_t status =
        snd_pe_get_export_address_by_hash(ntdll_base, SND_SYS_DLL_SIZE_DEFAULT, func_hash, &func_addr, NULL);
    if (status.code != SND_SUCCESS || !func_addr) {
        return status;
    }

    BYTE *p = (BYTE *)(ULONG_PTR)func_addr;

    PIMAGE_DOS_HEADER dos        = (PIMAGE_DOS_HEADER)ntdll_base;
    PIMAGE_NT_HEADERS nt         = (PIMAGE_NT_HEADERS)SND_PTR_ADD(ntdll_base, dos->e_lfanew);
    SIZE_T            image_size = nt->OptionalHeader.SizeOfImage;

    if (!snd_ptr_bounds_check(ntdll_base, image_size, p, 8)) {
        return SND_ERR(SND_STATUS_EXPORT_DIRECTORY_INVALID);
    }

    entry_out->dwHash   = func_hash;
    entry_out->pAddress = (PVOID)(ULONG_PTR)func_addr;

#if defined(_WIN64)
    if (p[0] == 0xE9 || p[0] == 0xEB || p[0] == 0xE8 || p[0] == 0xCC) {
        return snd_neighbor_search_ssn(p, ntdll_base, image_size, &entry_out->wSystemCall);
    } else if (p[3] == 0xE9 || p[3] == 0xEB || p[3] == 0xCC) {
        return snd_neighbor_search_ssn(p, ntdll_base, image_size, &entry_out->wSystemCall);
    } else if (p[0] == 0x4C && p[1] == 0x8B && p[2] == 0xD1 && p[3] == 0xB8) {
        entry_out->wSystemCall = *(WORD *)(p + 4);
        return SND_OK;
    }

#elif defined(_WIN32)
    if (p[0] == 0xB8) {
        entry_out->wSystemCall = *(WORD *)(p + 1);
        return SND_OK;
    }

    if (p[0] == 0xE9 || p[0] == 0xEB || p[0] == 0xCC) {
        return snd_neighbor_search_ssn(p, ntdll_base, image_size, &entry_out->wSystemCall);
    }
#endif

    return SND_ERR(SND_STATUS_SSN_NOT_FOUND);
}

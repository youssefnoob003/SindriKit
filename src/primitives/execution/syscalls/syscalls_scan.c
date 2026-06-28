#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>
#include <windows.h>

static snd_status_t neighbor_search_ssn(BYTE *p, PVOID ntdll, SIZE_T image_size, WORD *ssn_out) {
#if defined(_WIN64)
    const INT step = 32;
#define BOUNDS_SIZE 8
#elif defined(_WIN32)
    const INT step = 16;
#define BOUNDS_SIZE 4
#endif

    for (WORD idx = 1; idx <= 500; idx++) {
        BYTE *up_ptr = p + (idx * step);
        if (snd_memory_ptr_bounds_check(ntdll, image_size, up_ptr, BOUNDS_SIZE)) {
#if defined(_WIN64)
            if (up_ptr[0] == 0x4C && up_ptr[1] == 0x8B && up_ptr[2] == 0xD1 && up_ptr[3] == 0xB8) {
                *ssn_out = *(WORD *)(up_ptr + 4) - idx;
                return SND_OK;
            }
#elif defined(_WIN32)
            if (up_ptr[0] == 0xB8) {
                *ssn_out = *(WORD *)(up_ptr + 1) - idx;
                return SND_OK;
            }
#endif
        }

        BYTE *down_ptr = p - (idx * step);
        if (snd_memory_ptr_bounds_check(ntdll, image_size, down_ptr, BOUNDS_SIZE)) {
#if defined(_WIN64)
            if (down_ptr[0] == 0x4C && down_ptr[1] == 0x8B && down_ptr[2] == 0xD1 && down_ptr[3] == 0xB8) {
                *ssn_out = *(WORD *)(down_ptr + 4) + idx;
                return SND_OK;
            }
#elif defined(_WIN32)
            if (down_ptr[0] == 0xB8) {
                *ssn_out = *(WORD *)(down_ptr + 1) + idx;
                return SND_OK;
            }
#endif
        }
    }
    return SND_ERR(SND_STATUS_SSN_NOT_FOUND);
}

snd_status_t snd_syscall_resolve_ssn_scan(PVOID ntdll, DWORD func_hash, snd_syscall_entry_t *entry_out) {
    if (!entry_out) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    if (!ntdll) {
        return SND_ERR_CTX(SND_STATUS_NTDLL_NOT_INITIALIZED, "NTDLL target could not be localized");
    }

    FARPROC      func_addr = NULL;
    snd_status_t status = snd_pe_get_export_address_hash(ntdll, SND_SYS_DLL_SIZE_DEFAULT, func_hash, &func_addr, NULL);
    if (SND_FAILED(status) || !func_addr) {
        return status;
    }

    PIMAGE_DOS_HEADER dos        = (PIMAGE_DOS_HEADER)ntdll;
    PIMAGE_NT_HEADERS nt         = (PIMAGE_NT_HEADERS)SND_PTR_ADD(ntdll, dos->e_lfanew);
    SIZE_T            image_size = nt->OptionalHeader.SizeOfImage;
    BYTE             *p          = (BYTE *)(ULONG_PTR)func_addr;

    if (!snd_memory_ptr_bounds_check(ntdll, image_size, p, 32)) {
        return SND_ERR_CTX(SND_STATUS_CORRUPTED_STATE, "Syscall export address out of bounds");
    }

    entry_out->dwHash   = func_hash;
    entry_out->pAddress = (PVOID)(ULONG_PTR)func_addr;

#if defined(_WIN64)
    if (p[0] == 0x4C && p[1] == 0x8B && p[2] == 0xD1 && p[3] == 0xB8) {
        entry_out->wSystemCall = *(WORD *)(p + 4);
        return SND_OK;
    }

    if (p[3] == 0xB8 && p[6] == 0x00 && p[7] == 0x00) {
        entry_out->wSystemCall = *(WORD *)(p + 4);
        return SND_OK;
    }

#elif defined(_WIN32)
    if (p[0] == 0xB8) {
        entry_out->wSystemCall = *(WORD *)(p + 1);
        return SND_OK;
    }
#endif

    return neighbor_search_ssn(p, ntdll, image_size, &entry_out->wSystemCall);
}

#include <sindri/common/macros.h>
#include <sindri/common/opcodes.h>
#include <sindri/internal/windows/types.h>
#include <sindri/parsers/env/ntdll.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/status.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>

static snd_status_t neighbor_search_ssn(BYTE *p, PVOID ntdll, SIZE_T image_size, WORD *ssn_out) {
#if defined(_WIN64)
    const INT step = 32;
#define BOUNDS_SIZE 8
#elif defined(_WIN32)
    const INT step = 16;
#define BOUNDS_SIZE 4
#endif

    for (WORD idx = 1; idx <= SND_MAX_SYSCALLS; idx++) {
        BYTE *up_ptr = p + (idx * step);
        if (snd_memory_ptr_bounds_check(ntdll, image_size, up_ptr, BOUNDS_SIZE)) {
#if defined(_WIN64)
            if (up_ptr[0] == SND_OPCODE_REX_W_R && up_ptr[1] == SND_OPCODE_MOV_R64_RM64 &&
                up_ptr[2] == SND_MODRM_R10_RCX && up_ptr[3] == SND_OPCODE_MOV_EAX_IMM32) {
                *ssn_out = *(WORD *)(up_ptr + 4) - idx;
                return SND_OK;
            }
#elif defined(_WIN32)
            if (up_ptr[0] == SND_OPCODE_MOV_EAX_IMM32) {
                *ssn_out = *(WORD *)(up_ptr + 1) - idx;
                return SND_OK;
            }
#endif
        }

        BYTE *down_ptr = p - (idx * step);
        if (snd_memory_ptr_bounds_check(ntdll, image_size, down_ptr, BOUNDS_SIZE)) {
#if defined(_WIN64)
            if (down_ptr[0] == SND_OPCODE_REX_W_R && down_ptr[1] == SND_OPCODE_MOV_R64_RM64 &&
                down_ptr[2] == SND_MODRM_R10_RCX && down_ptr[3] == SND_OPCODE_MOV_EAX_IMM32) {
                *ssn_out = *(WORD *)(down_ptr + 4) + idx;
                return SND_OK;
            }
#elif defined(_WIN32)
            if (down_ptr[0] == SND_OPCODE_MOV_EAX_IMM32) {
                *ssn_out = *(WORD *)(down_ptr + 1) + idx;
                return SND_OK;
            }
#endif
        }
    }
    return SND_ERR_CTX(SND_STATUS_SSN_NOT_FOUND, "Syscall stub neighborhood scan failed to extract SSN");
}

snd_status_t snd_syscall_resolve_ssn_scan(DWORD func_hash, snd_syscall_entry_t *entry_out) {
    SND_CHECK_NULL(entry_out);

    FARPROC                func_addr  = NULL;
    const snd_pe_parser_t *parser_ptr = NULL;

    PVOID ntdll;
    SND_TRY(snd_ntdll_get_clean_base(&ntdll));
    SND_TRY(snd_ntdll_get_clean_parser(&parser_ptr));
    SND_TRY(snd_ntdll_get_clean_export(func_hash, &func_addr));

    if (!func_addr) {
        return SND_ERR_CTX(SND_STATUS_SSN_NOT_FOUND, "Export hash 0x%08X not found in target NTDLL", func_hash);
    }

    SIZE_T image_size = SND_PE_GET_NT_FIELD(parser_ptr, OptionalHeader.SizeOfImage);
    BYTE  *p          = (BYTE *)(ULONG_PTR)func_addr;

    if (!snd_memory_ptr_bounds_check(ntdll, image_size, p, 32)) {
        return SND_ERR_CTX(SND_STATUS_SSN_NOT_FOUND, "Syscall export address out of bounds");
    }

    entry_out->dwHash   = func_hash;
    entry_out->pAddress = (PVOID)(ULONG_PTR)func_addr;

#if defined(_WIN64)
    if (p[0] == SND_OPCODE_REX_W_R && p[1] == SND_OPCODE_MOV_R64_RM64 && p[2] == SND_MODRM_R10_RCX &&
        p[3] == SND_OPCODE_MOV_EAX_IMM32) {
        entry_out->wSystemCall = *(WORD *)(p + 4);
        return SND_OK;
    }

    if (p[3] == SND_OPCODE_MOV_EAX_IMM32 && p[6] == 0x00 && p[7] == 0x00) {
        entry_out->wSystemCall = *(WORD *)(p + 4);
        return SND_OK;
    }

#elif defined(_WIN32)
    if (p[0] == SND_OPCODE_MOV_EAX_IMM32) {
        entry_out->wSystemCall = *(WORD *)(p + 1);
        return SND_OK;
    }
#endif

    return neighbor_search_ssn(p, ntdll, image_size, &entry_out->wSystemCall);
}

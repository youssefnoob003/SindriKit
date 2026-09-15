#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/common/opcodes.h>
#include <sindri/internal/windows/types.h>
#include <sindri/internal/nt/base.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/status.h>
#include <sindri/primitives/syscalls.h>
#include <sindri_hashes.h>


snd_status_t snd_syscall_find_spoof_scan(snd_syscall_entry_t *entry) {
    SND_CHECK_NULL(entry, entry->pSyscallAddr);

    PVOID kernel32 = NULL;
    SND_TRY(snd_peb_get_module_base_hash(SND_HASH_KERNEL32_DLL, &kernel32));

    PVOID gadget = NULL;

#if defined(_WIN64)
    snd_buffer_t buf = {.data = kernel32, .size = SND_SYS_DLL_SIZE_DEFAULT};

    snd_pe_parser_t parser = {0};
    SND_TRY(snd_pe_parse(&buf, TRUE, &parser));

    DWORD pdata_rva = parser.nt.nt64->OptionalHeader.DataDirectory[SND_IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
    DWORD pdata_sz  = parser.nt.nt64->OptionalHeader.DataDirectory[SND_IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;

    if (pdata_rva && pdata_sz) {
        PSND_IMAGE_RUNTIME_FUNCTION pdata = (PSND_IMAGE_RUNTIME_FUNCTION)SND_PTR_ADD(kernel32, pdata_rva);
        DWORD                       count = pdata_sz / sizeof(*pdata);

        static ULONG call_count = 0;
        ULONG system_time = *(volatile ULONG*)SND_KUSER_SHARED_DATA_SYSTEM_TIME;
        ULONG entropy     = (ULONG)((ULONG_PTR)kernel32 & 0xFFFFFFFF) ^ (ULONG)((ULONG_PTR)entry & 0xFFFFFFFF) ^ system_time;
        ULONG skip_count  = (entry->dwHash + ++call_count + entropy) % 15;
        ULONG        current_matches = 0;

        PVOID fallback_gadget     = NULL;
        DWORD fallback_frame_size = 0;

        for (DWORD i = 0; i < count; i++) {
            DWORD begin      = pdata[i].BeginAddress;
            DWORD end        = pdata[i].EndAddress;
            DWORD unwind_rva = pdata[i].u.UnwindInfoAddress;

            if (!unwind_rva || (unwind_rva & 3)) {
                continue;
            }

            PBYTE unwind_info = (PBYTE)SND_PTR_ADD(kernel32, unwind_rva);

            BYTE version = unwind_info[0] & 7;
            BYTE flags   = unwind_info[0] >> 3;

            // Reject non-standard versions or chained unwind info
            if ((version != 1 && version != 2) || (flags & 4) != 0) {
                continue;
            }

            // Reject functions using a frame pointer (RBP) to prevent unwinder desync
            BYTE frame_reg = unwind_info[3] & 0x0F;
            if (frame_reg != 0) {
                continue;
            }

            BYTE  code_count = unwind_info[2];
            DWORD frame_size = 0;

            if (code_count > 0) {
                PWORD codes = (PWORD)(unwind_info + 4);

                for (BYTE c = 0; c < code_count; c++) {
                    BYTE op   = (codes[c] >> 8) & 0x0F;
                    BYTE info = (codes[c] >> 12) & 0x0F;

                    switch (op) {
                    case 0: // UWOP_PUSH_NONVOL
                        frame_size += 8;
                        break;

                    case 1: // UWOP_ALLOC_LARGE
                        if (info == 0) {
                            c++;
                            if (c < code_count)
                                frame_size += codes[c] * 8;
                        } else if (info == 1) {
                            c += 2;
                            if (c < code_count)
                                frame_size += ((DWORD)codes[c - 1] | ((DWORD)codes[c] << 16));
                        }
                        break;

                    case 2: // UWOP_ALLOC_SMALL
                        frame_size += (info + 1) * 8;
                        break;

                    case 3:  // UWOP_SET_FPREG
                    case 10: // UWOP_PUSH_MACHFRAME
                        break;

                    case 4: // UWOP_SAVE_NONVOL
                    case 6: // UWOP_EPILOG
                    case 8: // UWOP_SAVE_XMM128
                        c++;
                        break;

                    case 5: // UWOP_SAVE_NONVOL_FAR
                    case 9: // UWOP_SAVE_XMM128_FAR
                        c += 2;
                        break;

                    default:
                        break;
                    }
                }
            }

            frame_size += 8; // Return address slot

            // Constrain frame size between SND_SPOOF_FRAME_MIN_SIZE and SND_SPOOF_FRAME_MAX_SIZE bytes
            if (frame_size >= SND_SPOOF_FRAME_MIN_SIZE && frame_size <= SND_SPOOF_FRAME_MAX_SIZE) {
                PBYTE fn_start = (PBYTE)SND_PTR_ADD(kernel32, begin);
                PBYTE fn_end   = (PBYTE)SND_PTR_ADD(kernel32, end);

                if (fn_end <= fn_start || (fn_end - fn_start) > SND_SPOOF_MAX_FUNCTION_SIZE) {
                    continue;
                }

                PVOID found_in_fn = NULL;
                for (PBYTE p = fn_start; p < fn_end; p++) {
                    if (p[0] == SND_OPCODE_RET) {
                        found_in_fn = p;
                        break;
                    }
                }

                if (found_in_fn) {
                    if (!fallback_gadget) {
                        fallback_gadget     = found_in_fn;
                        fallback_frame_size = frame_size;
                    }

                    if (current_matches < skip_count) {
                        current_matches++;
                        continue;
                    }

                    gadget                  = found_in_fn;
                    entry->dwSpoofFrameSize = frame_size;
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

    snd_buffer_t    k32_buf    = {.data = kernel32, .size = SND_SYS_DLL_SIZE_DEFAULT};
    snd_pe_parser_t k32_parser = {0};

    SND_TRY(snd_pe_parse(&k32_buf, TRUE, &k32_parser));
    SND_TRY(snd_pe_get_export_address_hash(&k32_parser, SND_HASH_BASETHREADINITTHUNK, &exec_addr, NULL));

    PBYTE scanner = (PBYTE)exec_addr;
    for (int i = 0; i < SND_SPOOF_MAX_GADGET_SCAN; i++) {
        if (scanner[i] == SND_OPCODE_RET) {
            gadget = &scanner[i];
            break;
        }
        if (scanner[-i] == SND_OPCODE_RET) {
            gadget = &scanner[-i];
            break;
        }
    }
    entry->dwSpoofFrameSize = 0;
#endif

    if (!gadget) {
        return SND_ERR_CTX(SND_STATUS_SPOOF_GADGET_NOT_FOUND, "No valid spoof gadget found in kernel32");
    }

    entry->pSpoofAddr = gadget;

    DWORD gadget_rva = (DWORD)SND_PTR_DELTA(gadget, kernel32);
    (void)gadget_rva;
    SND_DEBUG_PRINT("[+] Spoof Gadget RVA: 0x%X (Frame Size: 0x%X)\n", gadget_rva, entry->dwSpoofFrameSize);

    return SND_OK;
}

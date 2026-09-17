#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/common/opcodes.h>
#include <sindri/internal/nt/base.h>
#include <sindri/internal/nt/process.h>
#include <sindri/internal/windows/coff.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/loaders/coff/engine.h>
#include <sindri/loaders/coff/status.h>
#include <sindri/parsers/coff.h>
#include <sindri/status/core.h>

snd_status_t snd_ldr_coff_allocate_and_copy_sections(snd_ldr_coff_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->mem_api, ctx->mem_api->alloc);

    if (ctx->stage < SND_COFF_STAGE_PARSED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected stage >= %s, got %s",
                           snd_ldr_coff_stage_to_string(SND_COFF_STAGE_PARSED),
                           snd_ldr_coff_stage_to_string(ctx->stage));
    }

    DWORD num_sections = ctx->coff.sections_count;
    if (num_sections == 0) {
        ctx->stage = SND_COFF_STAGE_SECTIONS_MAPPED;
        return SND_OK;
    }

    SIZE_T ptr_size = sizeof(LPVOID);

    if (SND_MUL_OVERFLOWS_SIZET(num_sections + 1, ptr_size) ||
        SND_MUL_OVERFLOWS_SIZET(ctx->coff.symbol_count, ptr_size)) {
        return SND_ERR(SND_STATUS_COFF_LOADER_MAP_SIZE_OVERFLOW);
    }

    SIZE_T section_map_size = SND_ALIGN_UP((num_sections + 1) * ptr_size, SND_COFF_METADATA_ALIGNMENT);
    SIZE_T symbol_map_size  = SND_ALIGN_UP(ctx->coff.symbol_count * ptr_size, SND_COFF_METADATA_ALIGNMENT);

    DWORD  num_externals = 0;
    SIZE_T bss_size      = 0;

    for (DWORD i = 0; i < ctx->coff.symbol_count; i++) {
        PSND_IMAGE_SYMBOL sym = snd_coff_get_symbol_by_index(&ctx->coff, i);
        if (sym) {
            snd_coff_decoded_sym_t decoded = {0};
            if (SND_SUCCEEDED(snd_coff_decode_symbol(&ctx->coff, sym, &decoded))) {
                if (decoded.type == SND_COFF_SYM_TYPE_IMPORT) {
                    num_externals++;
                } else if (decoded.type == SND_COFF_SYM_TYPE_BSS) {
                    if (SND_ADD_OVERFLOWS_SIZET(bss_size, decoded.bss_size)) {
                        return SND_ERR(SND_STATUS_COFF_LOADER_BSS_SIZE_OVERFLOW);
                    }
                    bss_size += decoded.bss_size;
                }
            }

            if (sym->NumberOfAuxSymbols >= (ctx->coff.symbol_count - i)) {
                return SND_ERR(SND_STATUS_SYMBOL_ENTRY_OUT_OF_RANGE);
            }
            i += sym->NumberOfAuxSymbols;
        }
    }

    if (SND_MUL_OVERFLOWS_SIZET(num_externals, ptr_size) ||
        (ctx->coff.is_64bit && SND_MUL_OVERFLOWS_SIZET(num_externals, SND_X64_TRAMPOLINE_SIZE))) {
        return SND_ERR(SND_STATUS_COFF_LOADER_TRAMPOLINE_OVERFLOW);
    }

    SIZE_T iat_size   = SND_ALIGN_UP(num_externals * ptr_size, SND_COFF_METADATA_ALIGNMENT);
    SIZE_T tramp_size = ctx->coff.is_64bit ? (num_externals * SND_X64_TRAMPOLINE_SIZE) : 0;

    SIZE_T total_virtual_size = 0;
    for (DWORD i = 0; i < num_sections; i++) {
        PSND_IMAGE_SECTION_HEADER sec = &ctx->coff.section_head[i];

        SIZE_T sec_size = sec->SizeOfRawData;
        if (sec->Misc.VirtualSize > sec_size) {
            sec_size = sec->Misc.VirtualSize;
        }

        if (sec_size > 0) {
            total_virtual_size = SND_ALIGN_UP(total_virtual_size, SND_PAGE_SIZE);
            if (SND_ADD_OVERFLOWS_SIZET(total_virtual_size, sec_size)) {
                return SND_ERR(SND_STATUS_COFF_LOADER_VIRTUAL_SIZE_OVERFLOW);
            }
            total_virtual_size += sec_size;
        }
    }

    SIZE_T map_offset = SND_ALIGN_UP(total_virtual_size, SND_PAGE_SIZE);

    if (SND_ADD_OVERFLOWS_SIZET(section_map_size, symbol_map_size) ||
        SND_ADD_OVERFLOWS_SIZET(section_map_size + symbol_map_size, iat_size) ||
        SND_ADD_OVERFLOWS_SIZET(section_map_size + symbol_map_size + iat_size, bss_size)) {
        return SND_ERR(SND_STATUS_COFF_LOADER_MAP_SIZE_OVERFLOW);
    }

    SIZE_T rw_metadata_size    = section_map_size + symbol_map_size + iat_size + bss_size;
    SIZE_T aligned_rw_metadata = SND_ALIGN_UP(rw_metadata_size, SND_PAGE_SIZE);
    SIZE_T aligned_tramp_size  = (tramp_size > 0) ? SND_ALIGN_UP(tramp_size, SND_PAGE_SIZE) : 0;

    if (SND_ADD_OVERFLOWS_SIZET(map_offset, aligned_rw_metadata) ||
        SND_ADD_OVERFLOWS_SIZET(map_offset + aligned_rw_metadata, aligned_tramp_size)) {
        return SND_ERR(SND_STATUS_COFF_LOADER_TRAMPOLINE_OVERFLOW);
    }

    SIZE_T total_alloc = map_offset + aligned_rw_metadata + aligned_tramp_size;

    LPVOID local_base = NULL;
    SND_TRY(ctx->mem_api->alloc(NULL, total_alloc, SND_MEM_COMMIT | SND_MEM_RESERVE, SND_PAGE_EXECUTE_READWRITE,
                                &local_base));

    ctx->target.local_base     = local_base;
    ctx->target.execution_base = local_base;
    ctx->target.allocated_size = total_alloc;

    ctx->target.section_map = (LPVOID *)SND_PTR_ADD(local_base, map_offset);
    ctx->target.symbol_map  = (LPVOID *)SND_PTR_ADD(ctx->target.section_map, section_map_size);
    ctx->target.iat_base    = SND_PTR_ADD(ctx->target.symbol_map, symbol_map_size);
    ctx->target.bss_base    = SND_PTR_ADD(ctx->target.iat_base, iat_size);

    ctx->target.trampolines_base = (tramp_size > 0) ? SND_PTR_ADD(local_base, map_offset + aligned_rw_metadata) : NULL;

    snd_memzero(ctx->target.section_map, section_map_size);
    snd_memzero(ctx->target.symbol_map, symbol_map_size);
    snd_memzero(ctx->target.iat_base, iat_size);
    snd_memzero(ctx->target.bss_base, bss_size);
    if (tramp_size > 0) {
        snd_memzero(ctx->target.trampolines_base, tramp_size);
    }

    SIZE_T current_offset = 0;
    for (DWORD i = 0; i < num_sections; i++) {
        PSND_IMAGE_SECTION_HEADER sec = &ctx->coff.section_head[i];

        SIZE_T sec_size = sec->SizeOfRawData;
        if (sec->Misc.VirtualSize > sec_size) {
            sec_size = sec->Misc.VirtualSize;
        }

        if (sec_size > 0) {
            current_offset = SND_ALIGN_UP(current_offset, SND_PAGE_SIZE);

            LPVOID dest                    = SND_PTR_ADD(local_base, current_offset);
            ctx->target.section_map[i + 1] = dest;

            if (sec->SizeOfRawData > 0) {
                PVOID src = snd_coff_raw_to_ptr(&ctx->coff, sec->PointerToRawData, sec->SizeOfRawData);
                if (src) {
                    snd_memcpy(dest, src, sec->SizeOfRawData);
                }
            }

            current_offset += sec_size;
        } else {
            ctx->target.section_map[i + 1] = NULL;
        }
    }

    ctx->stage = SND_COFF_STAGE_SECTIONS_MAPPED;
    return SND_OK;
}

snd_status_t snd_ldr_coff_resolve_symbols(snd_ldr_coff_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->mod_api, ctx->mod_api->load_library, ctx->mod_api->get_proc_address);

    if (ctx->stage != SND_COFF_STAGE_SECTIONS_MAPPED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Sequence error: Expected %s stage, got %s",
                           snd_ldr_coff_stage_to_string(SND_COFF_STAGE_SECTIONS_MAPPED),
                           snd_ldr_coff_stage_to_string(ctx->stage));
    }

    if (ctx->coff.symbol_count == 0) {
        ctx->stage = SND_COFF_STAGE_SYMBOLS_RESOLVED;
        return SND_OK;
    }

    SIZE_T iat_offset   = 0;
    SIZE_T tramp_offset = 0;
    SIZE_T bss_offset   = 0;
    SIZE_T ptr_size     = ctx->coff.is_64bit ? sizeof(ULONGLONG) : sizeof(DWORD);

    for (DWORD i = 0; i < ctx->coff.symbol_count; i++) {
        PSND_IMAGE_SYMBOL sym = snd_coff_get_symbol_by_index(&ctx->coff, i);
        if (!sym) {
            return SND_ERR(SND_STATUS_COFF_LOADER_SYMBOL_MISSING);
        }

        snd_coff_decoded_sym_t decoded = {0};
        SND_TRY(snd_coff_decode_symbol(&ctx->coff, sym, &decoded));

        if (decoded.type == SND_COFF_SYM_TYPE_IMPORT) {
            HMODULE hMod = NULL;
            FARPROC proc = NULL;

            SND_TRY(ctx->mod_api->load_library(decoded.import.dll_name, &hMod));
            SND_TRY(ctx->mod_api->get_proc_address(hMod, decoded.import.func_name, &proc));

            if (decoded.import.is_imp) {
                BYTE *iat_slot = SND_PTR_ADD(ctx->target.iat_base, iat_offset);

                if (ctx->coff.is_64bit) {
                    *(ULONGLONG *)iat_slot = (ULONGLONG)(ULONG_PTR)proc;
                } else {
                    *(DWORD *)iat_slot = (DWORD)(ULONG_PTR)proc;
                }

                ctx->target.symbol_map[i] = iat_slot;
                iat_offset += ptr_size;
            } else {
                if (ctx->coff.is_64bit) {
                    BYTE *tramp = (BYTE *)SND_PTR_ADD(ctx->target.trampolines_base, tramp_offset);

                    tramp[0]                                                 = SND_OPCODE_JMP_INDIRECT_0;
                    tramp[1]                                                 = SND_OPCODE_JMP_INDIRECT_1;
                    *(INT32 *)(tramp + SND_X64_TRAMPOLINE_DISP_OFFSET)       = 0;
                    *(ULONGLONG *)(tramp + SND_X64_TRAMPOLINE_TARGET_OFFSET) = (ULONGLONG)(ULONG_PTR)proc;

                    ctx->target.symbol_map[i] = tramp;
                    tramp_offset += SND_X64_TRAMPOLINE_SIZE;
                } else {
                    ctx->target.symbol_map[i] = (LPVOID)proc;
                }
            }
        } else if (decoded.type == SND_COFF_SYM_TYPE_BSS) {
            ctx->target.symbol_map[i] = SND_PTR_ADD(ctx->target.bss_base, bss_offset);
            bss_offset += decoded.bss_size;
        } else if (decoded.type == SND_COFF_SYM_TYPE_LOCAL) {
            if (sym->SectionNumber > 0 && (DWORD)sym->SectionNumber <= ctx->coff.sections_count) {
                LPVOID sym_section_base   = ctx->target.section_map[sym->SectionNumber];
                ctx->target.symbol_map[i] = SND_PTR_ADD(sym_section_base, sym->Value);
            } else {
                ctx->target.symbol_map[i] = (LPVOID)(ULONG_PTR)sym->Value;
            }
        } else {
            ctx->target.symbol_map[i] = (LPVOID)(ULONG_PTR)sym->Value;
        }

        if (sym->NumberOfAuxSymbols >= (ctx->coff.symbol_count - i)) {
            return SND_ERR(SND_STATUS_SYMBOL_ENTRY_OUT_OF_RANGE);
        }
        i += sym->NumberOfAuxSymbols;
    }

    ctx->stage = SND_COFF_STAGE_SYMBOLS_RESOLVED;
    return SND_OK;
}

snd_status_t snd_ldr_coff_apply_relocations(snd_ldr_coff_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->target.section_map, ctx->target.symbol_map);

    if (ctx->stage < SND_COFF_STAGE_SYMBOLS_RESOLVED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Sequence error: Expected stage >= %s, got %s",
                           snd_ldr_coff_stage_to_string(SND_COFF_STAGE_SYMBOLS_RESOLVED),
                           snd_ldr_coff_stage_to_string(ctx->stage));
    }

    const snd_coff_parser_t *parser = &ctx->coff;

    for (DWORD i = 0; i < parser->sections_count; i++) {
        PSND_IMAGE_SECTION_HEADER sec = &parser->section_head[i];

        PSND_IMAGE_RELOCATION relocs      = NULL;
        DWORD                 reloc_count = 0;

        SND_TRY(snd_coff_get_relocations(parser, sec, &relocs, &reloc_count));

        if (reloc_count == 0) {
            continue;
        }

        LPVOID section_dest = ctx->target.section_map[i + 1];
        if (!section_dest) {
            continue;
        }

        SIZE_T sec_size = sec->SizeOfRawData;
        if (sec->Misc.VirtualSize > sec_size) {
            sec_size = sec->Misc.VirtualSize;
        }

        for (DWORD j = 0; j < reloc_count; j++) {
            PSND_IMAGE_RELOCATION rel = &relocs[j];

            if (rel->SymbolTableIndex >= parser->symbol_count) {
                return SND_ERR(SND_STATUS_COFF_LOADER_SYMBOL_MISSING);
            }

            ULONG_PTR target_addr = (ULONG_PTR)ctx->target.symbol_map[rel->SymbolTableIndex];
            if (target_addr == 0) {
                return SND_ERR(SND_STATUS_SYMBOL_ADDRESS_NULL);
            }

            ULONG_PTR remote_target_addr = target_addr;
            if (ctx->target.execution_base && target_addr >= (ULONG_PTR)ctx->target.local_base &&
                target_addr < (ULONG_PTR)ctx->target.local_base + ctx->target.allocated_size) {
                remote_target_addr =
                    (ULONG_PTR)ctx->target.execution_base + SND_PTR_DELTA(target_addr, ctx->target.local_base);
            }

            SIZE_T reloc_size = 0;
            SND_TRY(snd_coff_get_relocation_patch_size(parser->is_64bit, rel->Type, &reloc_size));

            DWORD reloc_rva = rel->u.VirtualAddress;
            if (sec_size == 0 || reloc_rva >= sec_size || reloc_size > (sec_size - reloc_rva)) {
                return SND_ERR(SND_STATUS_COFF_LOADER_RELOC_OUT_OF_RANGE);
            }

            ULONG_PTR reloc_target = (ULONG_PTR)section_dest + reloc_rva;

            ULONG_PTR remote_reloc_target = reloc_target;
            if (ctx->target.execution_base) {
                remote_reloc_target =
                    (ULONG_PTR)ctx->target.execution_base + SND_PTR_DELTA(reloc_target, ctx->target.local_base);
            }

            if (parser->is_64bit) {
                switch (rel->Type) {
                case SND_IMAGE_REL_AMD64_ADDR64:
                    *(ULONG64 *)reloc_target += (ULONG64)remote_target_addr;
                    break;
                case SND_IMAGE_REL_AMD64_ADDR32NB:
                    *(DWORD *)reloc_target += (DWORD)(SND_PTR_DELTA(remote_target_addr, ctx->target.execution_base));
                    break;
                case SND_IMAGE_REL_AMD64_REL32:
                case SND_IMAGE_REL_AMD64_REL32_1:
                case SND_IMAGE_REL_AMD64_REL32_2:
                case SND_IMAGE_REL_AMD64_REL32_3:
                case SND_IMAGE_REL_AMD64_REL32_4:
                case SND_IMAGE_REL_AMD64_REL32_5: {
                    DWORD    disp  = SND_REL32_DISP_SIZE + (rel->Type - SND_IMAGE_REL_AMD64_REL32);
                    LONGLONG delta = (LONGLONG)(SND_PTR_DELTA(remote_target_addr, (remote_reloc_target + disp)));
                    if (delta < INT32_MIN || delta > INT32_MAX) {
                        return SND_ERR(SND_STATUS_COFF_LOADER_RELOC_OUT_OF_RANGE);
                    }

                    *(LONG32 *)reloc_target += (LONG32)delta;
                    break;
                }
                default:
                    return SND_ERR(SND_STATUS_RELOCATION_UNKNOWN_TYPE);
                }
            } else {
                switch (rel->Type) {
                case SND_IMAGE_REL_I386_DIR32:
                    *(DWORD *)reloc_target += (DWORD)remote_target_addr;
                    break;
                case SND_IMAGE_REL_I386_DIR32NB:
                    *(DWORD *)reloc_target += (DWORD)SND_PTR_DELTA(remote_target_addr, ctx->target.execution_base);
                    break;
                case SND_IMAGE_REL_I386_REL32:
                    *(DWORD *)reloc_target +=
                        (DWORD)SND_PTR_DELTA(remote_target_addr, (remote_reloc_target + SND_REL32_DISP_SIZE));
                    break;
                default:
                    return SND_ERR(SND_STATUS_RELOCATION_UNKNOWN_TYPE);
                }
            }
        }
    }

    ctx->stage = SND_COFF_STAGE_RELOCATED;
    return SND_OK;
}

snd_status_t snd_ldr_coff_apply_memory_protections(snd_ldr_coff_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->mem_api, ctx->mem_api->protect);

    if (ctx->stage < SND_COFF_STAGE_RELOCATED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Expected stage >= %s, got %s",
                           snd_ldr_coff_stage_to_string(SND_COFF_STAGE_RELOCATED),
                           snd_ldr_coff_stage_to_string(ctx->stage));
    }

    for (DWORD i = 0; i < ctx->coff.sections_count; i++) {
        PSND_IMAGE_SECTION_HEADER sec          = &ctx->coff.section_head[i];
        LPVOID                    section_dest = ctx->target.section_map[i + 1];

        if (!section_dest)
            continue;

        DWORD protect = SND_PAGE_READONLY;

        if ((sec->Characteristics & SND_IMAGE_SCN_MEM_EXECUTE) && (sec->Characteristics & SND_IMAGE_SCN_MEM_WRITE)) {
            protect = SND_PAGE_EXECUTE_READWRITE;
        } else if (sec->Characteristics & SND_IMAGE_SCN_MEM_EXECUTE) {
            protect = SND_PAGE_EXECUTE_READ;
        } else if (sec->Characteristics & SND_IMAGE_SCN_MEM_WRITE) {
            protect = SND_PAGE_READWRITE;
        }

        SIZE_T sec_size = sec->SizeOfRawData;
        if (sec->Misc.VirtualSize > sec_size) {
            sec_size = sec->Misc.VirtualSize;
        }

        if (sec_size == 0) {
            continue;
        }

        SIZE_T aligned_size = SND_ALIGN_UP(sec_size, SND_PAGE_SIZE);
        DWORD  old_protect  = 0;

        SND_TRY(ctx->mem_api->protect(section_dest, aligned_size, protect, &old_protect));
    }

    if (ctx->target.section_map) {
        DWORD old_protect = 0;

        if (ctx->target.trampolines_base) {
            SIZE_T rw_size = SND_PTR_DELTA(ctx->target.trampolines_base, ctx->target.section_map);
            if (rw_size > 0) {
                SND_TRY(ctx->mem_api->protect(ctx->target.section_map, rw_size, SND_PAGE_READWRITE, &old_protect));
            }

            SIZE_T rx_size =
                ctx->target.allocated_size - (SND_PTR_DELTA(ctx->target.trampolines_base, ctx->target.local_base));
            if (rx_size > 0) {
                SND_TRY(
                    ctx->mem_api->protect(ctx->target.trampolines_base, rx_size, SND_PAGE_EXECUTE_READ, &old_protect));
            }
        } else {
            SIZE_T metadata_size =
                ctx->target.allocated_size - (SND_PTR_DELTA(ctx->target.section_map, ctx->target.local_base));
            SIZE_T aligned_metadata_size = SND_ALIGN_UP(metadata_size, SND_PAGE_SIZE);
            if (aligned_metadata_size > 0) {
                SND_TRY(ctx->mem_api->protect(ctx->target.section_map, aligned_metadata_size, SND_PAGE_READWRITE,
                                              &old_protect));
            }
        }
    }

    ctx->stage = SND_COFF_STAGE_READY_FOR_EXECUTION;
    return SND_OK;
}

void snd_ldr_coff_free_mapped_image(snd_ldr_coff_ctx_t *ctx) {
    if (ctx && ctx->target.local_base && ctx->mem_api && ctx->mem_api->free) {
        ctx->mem_api->free(ctx->target.local_base, 0, SND_MEM_RELEASE);
        snd_memzero(&ctx->target, sizeof(ctx->target));
        ctx->stage = SND_COFF_STAGE_UNINITIALIZED;
    }
}

#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/internal/nt/types.h>
#include <sindri/loaders/coff/engine.h>
#include <sindri/parsers/coff.h>
#include <windows.h>

snd_status_t snd_ldr_coff_allocate_and_copy_sections(snd_ldr_coff_ctx_t *ctx) {
    if (!ctx || !ctx->mem_api || !ctx->mem_api->alloc) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    if (ctx->stage < SND_COFF_STAGE_PARSED) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }

    DWORD num_sections = ctx->coff.sections_count;
    if (num_sections == 0) {
        ctx->stage = SND_COFF_STAGE_SECTIONS_MAPPED;
        return SND_OK;
    }

    /* Native pointer-size-agnostic sizing */
    SIZE_T ptr_size = sizeof(LPVOID);

    /* Align metadata block sizes to 16-byte boundaries to prevent AVX/SSE alignment crashes */
    SIZE_T section_map_size = (num_sections + 1) * ptr_size;
    section_map_size        = (section_map_size + 15) & ~15;

    SIZE_T symbol_map_size = ctx->coff.symbol_count * ptr_size;
    symbol_map_size        = (symbol_map_size + 15) & ~15;

    DWORD  num_externals = 0;
    SIZE_T bss_size      = 0;

    for (DWORD i = 0; i < ctx->coff.symbol_count; i++) {
        PIMAGE_SYMBOL sym = snd_coff_get_symbol_by_index(&ctx->coff, i);
        if (sym) {
            snd_coff_decoded_sym_t decoded = {0};
            if (SND_SUCCEEDED(snd_coff_decode_symbol(&ctx->coff, sym, &decoded))) {
                if (decoded.type == SND_COFF_SYM_TYPE_IMPORT) {
                    num_externals++;
                } else if (decoded.type == SND_COFF_SYM_TYPE_BSS) {
                    bss_size += decoded.bss_size;
                }
            }
            i += sym->NumberOfAuxSymbols;
        }
    }

    SIZE_T iat_size = num_externals * ptr_size;
    iat_size        = (iat_size + 15) & ~15;

    SIZE_T tramp_size = ctx->coff.is_64bit ? (num_externals * 16) : 0;

    /* Calculate the overall virtual size using the true maximum size of each section */
    SIZE_T total_virtual_size = 0;
    for (DWORD i = 0; i < num_sections; i++) {
        PIMAGE_SECTION_HEADER sec = &ctx->coff.section_head[i];

        SIZE_T sec_size = sec->SizeOfRawData;
        if (sec->Misc.VirtualSize > sec_size) {
            sec_size = sec->Misc.VirtualSize;
        }

        if (sec_size > 0) {
            total_virtual_size = (total_virtual_size + SND_PAGE_SIZE - 1) & ~(SND_PAGE_SIZE - 1);
            total_virtual_size += sec_size;
        }
    }

    SIZE_T map_offset = (total_virtual_size + SND_PAGE_SIZE - 1) & ~(SND_PAGE_SIZE - 1);

    SIZE_T rw_metadata_size    = section_map_size + symbol_map_size + iat_size + bss_size;
    SIZE_T aligned_rw_metadata = (rw_metadata_size + SND_PAGE_SIZE - 1) & ~(SND_PAGE_SIZE - 1);

    SIZE_T aligned_tramp_size = 0;
    if (tramp_size > 0) {
        aligned_tramp_size = (tramp_size + SND_PAGE_SIZE - 1) & ~(SND_PAGE_SIZE - 1);
    }

    SIZE_T total_alloc = map_offset + aligned_rw_metadata + aligned_tramp_size;

    LPVOID       local_base = NULL;
    snd_status_t status =
        ctx->mem_api->alloc(NULL, total_alloc, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE, &local_base);

    if (SND_FAILED(status) || local_base == NULL) {
        return SND_ERR(SND_STATUS_ALLOC_FAILED);
    }

    ctx->target.local_base     = local_base;
    ctx->target.execution_base = local_base;
    ctx->target.allocated_size = total_alloc;

    ctx->target.section_map = (LPVOID *)SND_PTR_ADD(local_base, map_offset);
    ctx->target.symbol_map  = (LPVOID *)SND_PTR_ADD(ctx->target.section_map, section_map_size);
    ctx->target.iat_base    = SND_PTR_ADD(ctx->target.symbol_map, symbol_map_size);
    ctx->target.bss_base    = SND_PTR_ADD(ctx->target.iat_base, iat_size);

    if (tramp_size > 0) {
        ctx->target.trampolines_base = SND_PTR_ADD(local_base, map_offset + aligned_rw_metadata);
    } else {
        ctx->target.trampolines_base = NULL;
    }

    snd_memzero(ctx->target.section_map, section_map_size);
    snd_memzero(ctx->target.symbol_map, symbol_map_size);
    snd_memzero(ctx->target.iat_base, iat_size);
    snd_memzero(ctx->target.bss_base, bss_size);
    if (tramp_size > 0) {
        snd_memzero(ctx->target.trampolines_base, tramp_size);
    }

    SIZE_T current_offset = 0;
    for (DWORD i = 0; i < num_sections; i++) {
        PIMAGE_SECTION_HEADER sec = &ctx->coff.section_head[i];

        SIZE_T sec_size = sec->SizeOfRawData;
        if (sec->Misc.VirtualSize > sec_size) {
            sec_size = sec->Misc.VirtualSize;
        }

        if (sec_size > 0) {
            current_offset = (current_offset + SND_PAGE_SIZE - 1) & ~(SND_PAGE_SIZE - 1);

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
            /* Zero-size sections should be mapped to NULL to prevent address space overlaps */
            ctx->target.section_map[i + 1] = NULL;
        }
    }

    ctx->stage = SND_COFF_STAGE_SECTIONS_MAPPED;
    return SND_OK;
}

snd_status_t snd_ldr_coff_resolve_symbols(snd_ldr_coff_ctx_t *ctx) {
    if (!ctx || !ctx->mod_api || !ctx->mod_api->load_library || !ctx->mod_api->get_proc_address) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    if (ctx->stage != SND_COFF_STAGE_SECTIONS_MAPPED) {
        return SND_ERR(SND_STATUS_INVALID_STAGE_SEQUENCE);
    }

    if (ctx->coff.symbol_count == 0) {
        ctx->stage = SND_COFF_STAGE_SYMBOLS_RESOLVED;
        return SND_OK;
    }

    SIZE_T iat_offset   = 0;
    SIZE_T tramp_offset = 0;
    SIZE_T bss_offset   = 0;
    SIZE_T ptr_size     = sizeof(LPVOID);

    for (DWORD i = 0; i < ctx->coff.symbol_count; i++) {
        PIMAGE_SYMBOL sym = snd_coff_get_symbol_by_index(&ctx->coff, i);
        if (!sym) {
            continue;
        }

        snd_coff_decoded_sym_t decoded = {0};
        snd_status_t           status  = snd_coff_decode_symbol(&ctx->coff, sym, &decoded);
        if (SND_FAILED(status)) {
            return status;
        }

        if (decoded.type == SND_COFF_SYM_TYPE_IMPORT) {
            HMODULE hMod = NULL;
            FARPROC proc = NULL;
            status       = ctx->mod_api->load_library(decoded.import.dll_name, &hMod);
            if (SND_SUCCEEDED(status) && hMod) {
                ctx->mod_api->get_proc_address(hMod, decoded.import.func_name, &proc);
            }

            if (!proc) {
                return SND_ERR(SND_STATUS_COFF_SYMBOL_RESOLUTION_FAILED);
            }

            if (decoded.import.is_imp) {
                LPVOID *iat_entry         = (LPVOID *)SND_PTR_ADD(ctx->target.iat_base, iat_offset);
                *iat_entry                = (LPVOID)proc;
                ctx->target.symbol_map[i] = (LPVOID)iat_entry;
                iat_offset += ptr_size;
            } else {
                if (ctx->coff.is_64bit) {
                    void *tramp = SND_PTR_ADD(ctx->target.trampolines_base, tramp_offset);
                    snd_coff_write_jmp_trampoline(tramp, (void *)proc);

                    ctx->target.symbol_map[i] = tramp;
                    tramp_offset += 16;
                } else {
                    ctx->target.symbol_map[i] = (LPVOID)proc;
                }
            }
        } else if (decoded.type == SND_COFF_SYM_TYPE_BSS) {
            ctx->target.symbol_map[i] = SND_PTR_ADD(ctx->target.bss_base, bss_offset);
            bss_offset += decoded.bss_size;
        } else if (decoded.type == SND_COFF_SYM_TYPE_LOCAL) {
            LPVOID sym_section_base   = ctx->target.section_map[sym->SectionNumber];
            ctx->target.symbol_map[i] = SND_PTR_ADD(sym_section_base, sym->Value);
        } else {
            ctx->target.symbol_map[i] = (LPVOID)(ULONG_PTR)sym->Value;
        }

        i += sym->NumberOfAuxSymbols;
    }

    ctx->stage = SND_COFF_STAGE_SYMBOLS_RESOLVED;
    return SND_OK;
}

snd_status_t snd_ldr_coff_apply_relocations(snd_ldr_coff_ctx_t *ctx) {
    if (!ctx) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }
    if (ctx->stage < SND_COFF_STAGE_SYMBOLS_RESOLVED) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }

    snd_status_t status = snd_coff_apply_relocations(&ctx->coff, ctx->target.section_map, ctx->target.symbol_map,
                                                     ctx->target.execution_base);

    if (SND_FAILED(status)) {
        return status;
    }

    ctx->stage = SND_COFF_STAGE_RELOCATED;
    return SND_OK;
}

snd_status_t snd_ldr_coff_apply_memory_protections(snd_ldr_coff_ctx_t *ctx) {
    if (!ctx || !ctx->mem_api || !ctx->mem_api->protect) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    if (ctx->stage < SND_COFF_STAGE_RELOCATED) {
        return SND_ERR(SND_STATUS_NOT_INITIALIZED);
    }

    for (DWORD i = 0; i < ctx->coff.sections_count; i++) {
        PIMAGE_SECTION_HEADER sec          = &ctx->coff.section_head[i];
        LPVOID                section_dest = ctx->target.section_map[i + 1];

        if (!section_dest)
            continue;

        DWORD protect = PAGE_READONLY;

        if ((sec->Characteristics & IMAGE_SCN_MEM_EXECUTE) && (sec->Characteristics & IMAGE_SCN_MEM_WRITE)) {
            protect = PAGE_EXECUTE_READWRITE;
        } else if (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            protect = PAGE_EXECUTE_READ;
        } else if (sec->Characteristics & IMAGE_SCN_MEM_WRITE) {
            protect = PAGE_READWRITE;
        }

        SIZE_T sec_size = sec->SizeOfRawData;
        if (sec->Misc.VirtualSize > sec_size) {
            sec_size = sec->Misc.VirtualSize;
        }

        if (sec_size == 0) {
            continue;
        }

        SIZE_T aligned_size = (sec_size + SND_PAGE_SIZE - 1) & ~(SND_PAGE_SIZE - 1);
        DWORD  old_protect  = 0;

        snd_status_t status = ctx->mem_api->protect(section_dest, aligned_size, protect, &old_protect);
        if (SND_FAILED(status)) {
            return SND_ERR(SND_STATUS_PROTECTION_UPDATE_FAILED);
        }
    }

    if (ctx->target.section_map) {
        DWORD        old_protect = 0;
        snd_status_t status      = SND_OK;

        if (ctx->target.trampolines_base) {
            SIZE_T rw_size = (ULONG_PTR)ctx->target.trampolines_base - (ULONG_PTR)ctx->target.section_map;
            if (rw_size > 0) {
                status = ctx->mem_api->protect(ctx->target.section_map, rw_size, PAGE_READWRITE, &old_protect);
                if (SND_FAILED(status))
                    return status;
            }

            SIZE_T rx_size = ctx->target.allocated_size -
                             ((ULONG_PTR)ctx->target.trampolines_base - (ULONG_PTR)ctx->target.local_base);
            if (rx_size > 0) {
                status = ctx->mem_api->protect(ctx->target.trampolines_base, rx_size, PAGE_EXECUTE_READ, &old_protect);
                if (SND_FAILED(status))
                    return status;
            }
        } else {
            SIZE_T metadata_size =
                ctx->target.allocated_size - ((ULONG_PTR)ctx->target.section_map - (ULONG_PTR)ctx->target.local_base);
            SIZE_T aligned_metadata_size = (metadata_size + SND_PAGE_SIZE - 1) & ~(SND_PAGE_SIZE - 1);
            if (aligned_metadata_size > 0) {
                status =
                    ctx->mem_api->protect(ctx->target.section_map, aligned_metadata_size, PAGE_READWRITE, &old_protect);
                if (SND_FAILED(status))
                    return status;
            }
        }
    }

    ctx->stage = SND_COFF_STAGE_READY_FOR_EXECUTION;
    return SND_OK;
}

void snd_ldr_coff_free_mapped_image(snd_ldr_coff_ctx_t *ctx) {
    if (ctx && ctx->target.local_base && ctx->mem_api && ctx->mem_api->free) {
        LPVOID base = ctx->target.local_base;
        SIZE_T size = 0;
        ctx->mem_api->free(base, size, MEM_RELEASE);
        ctx->target.local_base  = NULL;
        ctx->target.section_map = NULL;
    }
}

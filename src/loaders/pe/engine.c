#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/internal/nt/base.h>
#include <sindri/internal/nt/process.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/internal/windows/pe.h>
#include <sindri/loaders/pe/engine.h>
#include <sindri/loaders/pe/status.h>
#include <sindri/parsers/pe.h>

static snd_status_t copy_headers(const snd_ldr_pe_ctx_t *ctx) {
    DWORD headers_size = SND_PE_GET_NT_FIELD(&ctx->pe, OptionalHeader.SizeOfHeaders);
    if (!snd_memory_bounds_check(ctx->target.allocated_size, 0, headers_size)) {
        return SND_ERR(SND_STATUS_HEADERS_SIZE_INVALID);
    }

    SIZE_T file_size        = ctx->raw_source->size;
    SIZE_T header_copy_size = SND_MIN(headers_size, file_size);

    snd_memcpy(ctx->target.local_base, ctx->raw_source->data, header_copy_size);

    if (headers_size > header_copy_size) {
        snd_memzero(SND_PTR_ADD(ctx->target.local_base, header_copy_size), headers_size - header_copy_size);
    }

    return SND_OK;
}

static snd_status_t copy_sections(const snd_ldr_pe_ctx_t *ctx) {
    const snd_pe_parser_t *pe = &ctx->pe;
    if (pe->sections_count == 0) {
        return SND_OK;
    }

    if (pe->section_head == NULL) {
        return SND_ERR(SND_STATUS_SECTION_TABLE_MISSING);
    }

    BYTE  *file_data = (BYTE *)ctx->raw_source->data;
    SIZE_T file_size = ctx->raw_source->size;

    for (DWORD i = 0; i < pe->sections_count; i++) {
        PSND_IMAGE_SECTION_HEADER curr_section = &pe->section_head[i];

        DWORD  size_to_copy  = snd_pe_section_copy_size(curr_section);
        DWORD  loaded_size   = snd_pe_section_loaded_size(curr_section);
        SIZE_T target_offset = (SIZE_T)curr_section->VirtualAddress;

        if (!snd_memory_bounds_check(ctx->target.allocated_size, target_offset, loaded_size)) {
            char name[SND_PE_MAX_SECTION_NAME_LEN];
            snd_pe_section_name(pe, curr_section, name, sizeof(name));
            return SND_ERR_CTX(SND_STATUS_SECTION_SIZE_INVALID, "Section %s extends past target allocation bounds.",
                               name);
        }

        size_to_copy     = SND_MIN(size_to_copy, loaded_size);
        BYTE *target_mem = SND_PTR_ADD(ctx->target.local_base, target_offset);

        if (size_to_copy > 0) {
            SIZE_T source_offset = (SIZE_T)curr_section->PointerToRawData;

            if (source_offset >= file_size) {
                size_to_copy = 0;
            } else {
                SIZE_T avail = file_size - source_offset;
                if ((SIZE_T)size_to_copy > avail) {
                    size_to_copy = (DWORD)avail;
                }
            }

            if (size_to_copy > 0) {
                BYTE *source_mem = SND_PTR_ADD(file_data, source_offset);
                snd_memcpy(target_mem, source_mem, size_to_copy);
            }
        }

        if (loaded_size > size_to_copy) {
            snd_memzero(SND_PTR_ADD(target_mem, size_to_copy), loaded_size - size_to_copy);
        }
    }

    return SND_OK;
}

snd_status_t snd_ldr_pe_allocate_and_copy_image(snd_ldr_pe_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->mem_api, ctx->mem_api->alloc);

    if (ctx->stage != SND_STAGE_PARSED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Sequence error: Expected %s, got %s",
                           snd_ldr_pe_stage_to_string(SND_STAGE_PARSED), snd_ldr_pe_stage_to_string(ctx->stage));
    }

    if (!ctx->raw_source || !ctx->raw_source->data || ctx->raw_source->size == 0) {
        return SND_ERR_CTX(SND_STATUS_CORRUPTED_STAGE, "State is PARSED but raw_source is invalid.");
    }

    SIZE_T base = (SIZE_T)SND_PE_GET_NT_FIELD(&ctx->pe, OptionalHeader.ImageBase);
    SIZE_T size = (SIZE_T)SND_PE_GET_NT_FIELD(&ctx->pe, OptionalHeader.SizeOfImage);

    if (size == 0) {
        return SND_ERR_CTX(SND_STATUS_IMAGE_SIZE_NULL, "PE SizeOfImage is zero.");
    }

    snd_status_t status = ctx->mem_api->alloc((LPVOID)base, size, SND_MEM_COMMIT | SND_MEM_RESERVE, SND_PAGE_READWRITE,
                                              &ctx->target.local_base);
    if (SND_FAILED(status)) {
        SND_TRY(ctx->mem_api->alloc(NULL, size, SND_MEM_COMMIT | SND_MEM_RESERVE, SND_PAGE_READWRITE,
                                    &ctx->target.local_base));
    }

    if (!ctx->target.execution_base) {
        ctx->target.execution_base = ctx->target.local_base;
    }

    ctx->target.allocated_size = size;
    ctx->stage                 = SND_STAGE_MEM_ALLOCATED;

    status = copy_headers(ctx);
    if (SND_FAILED(status))
        goto err_cleanup;

    status = copy_sections(ctx);
    if (SND_FAILED(status))
        goto err_cleanup;

    snd_buffer_t    mapped_target_buf = {.data = ctx->target.local_base, .size = ctx->target.allocated_size};
    snd_pe_parser_t mapped_pe         = {0};

    status = snd_pe_parse(&mapped_target_buf, TRUE, &mapped_pe);
    if (SND_FAILED(status))
        goto err_cleanup;

    ctx->pe    = mapped_pe;
    ctx->stage = SND_STAGE_SECTIONS_MAPPED;
    return SND_OK;

err_cleanup:
    if (ctx->target.local_base) {
        if (ctx->mem_api->free) {
            ctx->mem_api->free(ctx->target.local_base, 0, SND_MEM_RELEASE);
        }
        ctx->target.local_base = NULL;
    }

    ctx->target.execution_base = NULL;
    ctx->target.allocated_size = 0;

    if (ctx->stage > SND_STAGE_PARSED) {
        ctx->stage = SND_STAGE_PARSED;
    }

    return status;
}

snd_status_t snd_ldr_pe_apply_relocations(snd_ldr_pe_ctx_t *ctx) {
    SND_CHECK_NULL(ctx);

    if (ctx->stage != SND_STAGE_SECTIONS_MAPPED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Sequence error: Expected %s, got %s",
                           snd_ldr_pe_stage_to_string(SND_STAGE_SECTIONS_MAPPED),
                           snd_ldr_pe_stage_to_string(ctx->stage));
    }

    if (!ctx->target.local_base || !ctx->target.execution_base || ctx->target.allocated_size == 0) {
        return SND_ERR_CTX(SND_STATUS_CORRUPTED_STAGE,
                           "State mismatch: Mapped stage requires valid target base and allocation size.");
    }

    const snd_pe_parser_t *parser         = &ctx->pe;
    SIZE_T                 preferred_base = (SIZE_T)SND_PE_GET_NT_FIELD(parser, OptionalHeader.ImageBase);
    ctx->target.delta_offset              = SND_PTR_DELTA(ctx->target.execution_base, preferred_base);

    LONG_PTR delta_offset = ctx->target.delta_offset;
    if (delta_offset == 0) {
        ctx->stage = SND_STAGE_RELOCATED;
        return SND_OK;
    }

    SND_IMAGE_DATA_DIRECTORY reloc_dir = {0};
    SND_TRY(snd_pe_get_directory(parser, SND_IMAGE_DIRECTORY_ENTRY_BASERELOC, &reloc_dir));

    if (reloc_dir.VirtualAddress == 0 || reloc_dir.Size == 0) {
        return SND_ERR(SND_STATUS_RELOCATION_DIRECTORY_MISSING);
    }

    BOOL relocs_stripped =
        (SND_PE_GET_NT_FIELD(parser, FileHeader.Characteristics) & SND_IMAGE_FILE_RELOCS_STRIPPED) != 0;
    if (relocs_stripped) {
        return SND_ERR(SND_STATUS_RELOCATION_DIRECTORY_STRIPPED);
    }

    SIZE_T cursor = 0;
    while (TRUE) {
        const SND_IMAGE_BASE_RELOCATION *block         = NULL;
        DWORD                            entries_count = 0;

        SND_TRY(snd_pe_get_reloc_block(parser, &cursor, &block, &entries_count));
        if (!block) {
            break;
        }

        for (DWORD i = 0; i < entries_count; i++) {
            snd_pe_reloc_entry_t entry;
            SND_TRY(snd_pe_get_reloc_entry(parser, block, i, &entry));

            if (entry.type == SND_IMAGE_REL_BASED_ABSOLUTE || !entry.patch_ptr) {
                continue;
            }

            switch (entry.type) {
            case SND_IMAGE_REL_BASED_HIGHLOW:
                *(DWORD *)entry.patch_ptr += (DWORD)delta_offset;
                break;
            case SND_IMAGE_REL_BASED_DIR64:
                *(ULONGLONG *)entry.patch_ptr += (ULONGLONG)(LONGLONG)delta_offset;
                break;
            case SND_IMAGE_REL_BASED_HIGH:
                *(WORD *)entry.patch_ptr += SND_HIGH_WORD(delta_offset);
                break;
            case SND_IMAGE_REL_BASED_LOW:
                *(WORD *)entry.patch_ptr += SND_LOW_WORD(delta_offset);
                break;
            default:
                return SND_ERR_CTX(SND_STATUS_RELOCATION_TYPE_INVALID, "Unsupported relocation type: %u", entry.type);
            }
        }
    }

    ctx->stage = SND_STAGE_RELOCATED;
    return SND_OK;
}

snd_status_t snd_ldr_pe_resolve_imports(snd_ldr_pe_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->mod_api, ctx->mod_api->load_library, ctx->mod_api->get_proc_address);

    if (ctx->stage != SND_STAGE_RELOCATED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Sequence error: Expected %s, got %s",
                           snd_ldr_pe_stage_to_string(SND_STAGE_RELOCATED), snd_ldr_pe_stage_to_string(ctx->stage));
    }

    if (!ctx->target.local_base) {
        return SND_ERR_CTX(SND_STATUS_CORRUPTED_STAGE, "State mismatch: Stage is %s but local_base is NULL.",
                           snd_ldr_pe_stage_to_string(SND_STAGE_RELOCATED));
    }

    const snd_pe_parser_t *parser = &ctx->pe;

    for (DWORD desc_idx = 0;; desc_idx++) {
        const SND_IMAGE_IMPORT_DESCRIPTOR *desc = NULL;
        SND_TRY(snd_pe_get_import_descriptor(parser, desc_idx, &desc));
        if (!desc) {
            break;
        }

        const char *dll_name = NULL;
        SND_TRY(snd_pe_get_import_name(parser, desc, &dll_name));

        HMODULE h_dll = NULL;
        SND_TRY(ctx->mod_api->load_library(dll_name, &h_dll));

        for (DWORD thunk_idx = 0;; thunk_idx++) {
            snd_pe_import_thunk_t thunk;
            SND_TRY(snd_pe_get_import_thunk(parser, desc, thunk_idx, &thunk));
            if (!thunk.iat_slot) {
                break;
            }

            FARPROC func_addr = NULL;
            LPCSTR  symbol    = thunk.is_ordinal ? (LPCSTR)(ULONG_PTR)thunk.ordinal : thunk.name;

            SND_TRY(ctx->mod_api->get_proc_address(h_dll, symbol, &func_addr));

            if (parser->is_64bit) {
                ((PSND_IMAGE_THUNK_DATA64)thunk.iat_slot)->u1.Function = (ULONGLONG)(ULONG_PTR)func_addr;
            } else {
                ((PSND_IMAGE_THUNK_DATA32)thunk.iat_slot)->u1.Function = (DWORD)(ULONG_PTR)func_addr;
            }
        }
    }

    ctx->stage = SND_STAGE_IMPORTS_RESOLVED;
    return SND_OK;
}

snd_status_t snd_ldr_pe_apply_memory_protections(snd_ldr_pe_ctx_t *ctx) {
    SND_CHECK_NULL(ctx, ctx->mem_api, ctx->mem_api->protect);

    if (ctx->stage != SND_STAGE_IMPORTS_RESOLVED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE, "Sequence error: Expected %s, got %s",
                           snd_ldr_pe_stage_to_string(SND_STAGE_IMPORTS_RESOLVED),
                           snd_ldr_pe_stage_to_string(ctx->stage));
    }

    if (!ctx->target.local_base) {
        return SND_ERR_CTX(SND_STATUS_CORRUPTED_STAGE, "State mismatch: Stage is %s but local_base is NULL.",
                           snd_ldr_pe_stage_to_string(SND_STAGE_IMPORTS_RESOLVED));
    }

    const snd_pe_parser_t *pe           = &ctx->pe;
    SIZE_T                 virtual_size = ctx->target.allocated_size;

    SIZE_T total_pages = virtual_size / SND_PAGE_SIZE + ((virtual_size % SND_PAGE_SIZE) ? 1 : 0);
    if (total_pages > 0) {
        SIZE_T run_start_page = 0;
        DWORD  run_protect    = snd_pe_get_page_protection_flags(pe, 0);
        DWORD  old_protect    = 0;

        for (SIZE_T p = 1; p <= total_pages; p++) {
            SIZE_T page_offset     = (p == total_pages) ? virtual_size : p * SND_PAGE_SIZE;
            DWORD  current_protect = (p < total_pages) ? snd_pe_get_page_protection_flags(pe, page_offset) : 0;

            if (p == total_pages || current_protect != run_protect) {
                SIZE_T run_offset = run_start_page * SND_PAGE_SIZE;
                SIZE_T run_length = (p - run_start_page) * SND_PAGE_SIZE;

                void *run_addr = SND_PTR_ADD(ctx->target.local_base, run_offset);
                SND_TRY(ctx->mem_api->protect(run_addr, run_length, run_protect, &old_protect));

                run_start_page = p;
                run_protect    = current_protect;
            }
        }
    }

    ctx->target.entry_point = snd_pe_get_entry_point(&ctx->pe);
    ctx->stage              = SND_STAGE_READY_FOR_EXECUTION;
    return SND_OK;
}

void snd_ldr_pe_free_mapped_image(snd_ldr_pe_ctx_t *ctx) {
    if (ctx != NULL && ctx->target.local_base != NULL) {
        if (ctx->mem_api && ctx->mem_api->free) {
            ctx->mem_api->free(ctx->target.local_base, 0, SND_MEM_RELEASE);
        }

        ctx->target.execution_base = NULL;
        ctx->target.local_base     = NULL;
        ctx->target.allocated_size = 0;
        ctx->target.entry_point    = NULL;

        if (ctx->raw_source != NULL && ctx->raw_source->data != NULL) {
            ctx->stage = SND_SUCCEEDED(snd_pe_parse(ctx->raw_source, FALSE, &ctx->pe)) ? SND_STAGE_PARSED
                                                                                       : SND_STAGE_UNINITIALIZED;
        } else {
            ctx->stage = SND_STAGE_UNINITIALIZED;
        }
    }
}

void snd_ldr_pe_execute_tls_callbacks(snd_ldr_pe_ctx_t *ctx, DWORD reason) {
    if (!ctx || ctx->stage < SND_STAGE_READY_FOR_EXECUTION || !ctx->target.local_base || !ctx->target.execution_base) {
        return;
    }

    if (ctx->target.local_base != ctx->target.execution_base) {
        return;
    }

    const snd_pe_parser_t *parser = &ctx->pe;
    if (!parser->source.data || !parser->is_mapped || !SND_IS_ARCH_COMPATIBLE(parser->is_64bit)) {
        return;
    }

    PVOID callbacks_ptr = snd_pe_get_tls_callbacks(parser);
    if (!callbacks_ptr) {
        return;
    }

    BYTE     *slot_ptr  = (BYTE *)callbacks_ptr;
    BYTE     *local_end = SND_PTR_ADD(ctx->target.local_base, ctx->target.allocated_size);
    SIZE_T    slot_size = parser->is_64bit ? sizeof(ULONGLONG) : sizeof(DWORD);
    ULONGLONG base_va   = (ULONGLONG)(ULONG_PTR)ctx->target.execution_base;

    while (slot_ptr + slot_size <= local_end) {
        ULONGLONG raw_cb_va = parser->is_64bit ? *(ULONGLONG *)slot_ptr : (ULONGLONG)(*(DWORD *)slot_ptr);
        if (raw_cb_va == 0) {
            break;
        }

        if (raw_cb_va >= base_va) {
            ULONGLONG cb_rva = raw_cb_va - base_va;
            if (cb_rva < ctx->target.allocated_size) {
                SND_PIMAGE_TLS_CALLBACK cb =
                    (SND_PIMAGE_TLS_CALLBACK)SND_PTR_ADD(ctx->target.local_base, (SIZE_T)cb_rva);
                cb(ctx->target.execution_base, reason, NULL);
            }
        }

        slot_ptr += slot_size;
    }
}

snd_status_t snd_ldr_pe_get_proc_address(const snd_ldr_pe_ctx_t *ctx, const char *func_name, FARPROC *func_addr_out) {
    SND_CHECK_NULL(ctx, func_name, func_addr_out);

    if (ctx->stage < SND_STAGE_READY_FOR_EXECUTION) {
        return SND_ERR_CTX(
            SND_STATUS_INVALID_STAGE, "Sequence error: Cannot resolve exports until image is %s. Current state: %s",
            snd_ldr_pe_stage_to_string(SND_STAGE_READY_FOR_EXECUTION), snd_ldr_pe_stage_to_string(ctx->stage));
    }

    return snd_pe_get_export_address(&ctx->pe, func_name, func_addr_out, ctx->mod_api->get_module_base);
}

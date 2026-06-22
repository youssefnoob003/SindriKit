#include "../../parsers/pe/pe_section_utils.h"

#include <sindri/common/helpers.h>
#include <sindri/common/status.h>
#include <sindri/internal/nt_defs.h>
#include <sindri/loaders/reflective/engine.h>
#include <sindri/parsers/pe.h>
#include <windows.h>

static inline DWORD get_protection_flags(DWORD characteristics) {
    static const DWORD protect_lookup[8] = {
        PAGE_NOACCESS,          // 000: No access flags set
        PAGE_READWRITE,         // 001: Write-only (Windows automatically elevates this to
                                // RW)
        PAGE_READONLY,          // 010: Read-only
        PAGE_READWRITE,         // 011: Read/Write
        PAGE_EXECUTE,           // 100: Execute-only
        PAGE_EXECUTE_READWRITE, // 101: Execute/Write (Windows elevates to ERW)
        PAGE_EXECUTE_READ,      // 110: Execute/Read
        PAGE_EXECUTE_READWRITE  // 111: Execute/Read/Write
    };

    DWORD index = ((characteristics & IMAGE_SCN_MEM_EXECUTE) ? 4 : 0) |
                  ((characteristics & IMAGE_SCN_MEM_READ) ? 2 : 0) | ((characteristics & IMAGE_SCN_MEM_WRITE) ? 1 : 0);

    return protect_lookup[index];
}

static const char *stage_to_string(snd_loader_stage_t stage) {
    switch (stage) {
    case SND_STAGE_UNINITIALIZED:
        return "UNINITIALIZED";
    case SND_STAGE_PARSED:
        return "PARSED";
    case SND_STAGE_MEM_ALLOCATED:
        return "MEM_ALLOCATED";
    case SND_STAGE_SECTIONS_MAPPED:
        return "SECTIONS_MAPPED";
    case SND_STAGE_RELOCATED:
        return "RELOCATED";
    case SND_STAGE_IMPORTS_RESOLVED:
        return "IMPORTS_RESOLVED";
    case SND_STAGE_READY_FOR_EXECUTION:
        return "READY_FOR_EXECUTION";
    case SND_STAGE_EXECUTED:
        return "EXECUTED";
    default:
        return "UNKNOWN_CORRUPTED";
    }
}

static snd_status_t engine_pe_copy_sections(PVOID virtual_base, const snd_buffer_t *raw_source,
                                            const snd_pe_parser_t *pe) {
    BYTE  *file_data = (BYTE *)raw_source->data;
    SIZE_T file_size = raw_source->size;

    SIZE_T image_size   = SND_PE_GET_NT_FIELD(pe, OptionalHeader.SizeOfImage);
    DWORD  headers_size = SND_PE_GET_NT_FIELD(pe, OptionalHeader.SizeOfHeaders);

    if (!snd_bounds_check(image_size, 0, headers_size)) {
        return SND_ERR_CTX(SND_STATUS_SECTION_COPY_FAILED, "Invalid or corrupt PE headers size calculation.");
    }
    if (file_size < headers_size) {
        return SND_ERR_CTX(SND_STATUS_SECTION_COPY_FAILED,
                           "Raw source file size is smaller than designated SizeOfHeaders.");
    }

    snd_memcpy(virtual_base, file_data, headers_size);

    for (DWORD i = 0; i < pe->sections_count; i++) {
        PIMAGE_SECTION_HEADER curr_section = &pe->section_head[i];
        char                  name[128];

        snd_i_section_name(pe, curr_section, name, sizeof(name));

        DWORD  size_to_copy  = snd_i_section_copy_size(curr_section);
        DWORD  loaded_size   = snd_i_section_loaded_size(curr_section);
        SIZE_T target_offset = (SIZE_T)curr_section->VirtualAddress;

        if (!snd_bounds_check(image_size, target_offset, loaded_size)) {
            return SND_ERR_CTX(SND_STATUS_SECTION_COPY_FAILED, "Section %s outside image bounds.", name);
        }

        BYTE *target_mem = SND_PTR_ADD(virtual_base, target_offset);

        if (size_to_copy > 0) {
            SIZE_T source_offset = (SIZE_T)curr_section->PointerToRawData;

            if (source_offset >= file_size) {
                size_to_copy = 0;
            } else if (size_to_copy > file_size - source_offset) {
                size_to_copy = (DWORD)(file_size - source_offset);
            }

            if (size_to_copy > 0) {
                BYTE *source_mem = SND_PTR_ADD(file_data, source_offset);
                snd_memcpy(target_mem, source_mem, size_to_copy);
            }
        }

        if (loaded_size > size_to_copy) {
            snd_memzero(target_mem + size_to_copy, loaded_size - size_to_copy);
        }
    }

    return SND_OK;
}

static snd_status_t engine_pe_apply_memory_protections(PVOID virtual_base, const snd_pe_parser_t *pe,
                                                       const snd_memory_api_t *mem_api) {
    DWORD  headers_size = SND_PE_GET_NT_FIELD(pe, OptionalHeader.SizeOfHeaders);
    SIZE_T image_size   = SND_PE_GET_NT_FIELD(pe, OptionalHeader.SizeOfImage);
    DWORD  old_protect  = 0;

    SIZE_T aligned_image_size = (image_size + SND_PAGE_SIZE - 1) & ~(SND_PAGE_SIZE - 1);

    SIZE_T max_virtual_address = headers_size;
    for (DWORD i = 0; i < pe->sections_count; i++) {
        PIMAGE_SECTION_HEADER section     = &pe->section_head[i];
        SIZE_T                section_end = (SIZE_T)section->VirtualAddress + snd_i_section_loaded_size(section);
        if (section_end > max_virtual_address && section_end <= image_size) {
            max_virtual_address = section_end;
        }
    }
    SIZE_T active_image_size = (max_virtual_address + SND_PAGE_SIZE - 1) & ~(SND_PAGE_SIZE - 1);
    if (active_image_size > aligned_image_size)
        active_image_size = aligned_image_size;

    SIZE_T current_run_start   = 0;
    DWORD  current_run_protect = 0;
    SIZE_T current_run_length  = 0;

    for (SIZE_T page_offset = 0; page_offset < aligned_image_size; page_offset += SND_PAGE_SIZE) {
        BOOL is_exec  = FALSE;
        BOOL is_read  = FALSE;
        BOOL is_write = FALSE;

        SIZE_T page_end = page_offset + SND_PAGE_SIZE;

        if (page_offset < headers_size) {
            is_read = TRUE;
        }

        if (page_offset < active_image_size) {
            for (DWORD i = 0; i < pe->sections_count; i++) {
                PIMAGE_SECTION_HEADER section = &pe->section_head[i];
                if (section->SizeOfRawData == 0 && section->Misc.VirtualSize == 0)
                    continue;

                SIZE_T section_start = (SIZE_T)section->VirtualAddress;
                SIZE_T section_size  = (SIZE_T)snd_i_section_loaded_size(section);

                if (!snd_bounds_check(image_size, section_start, section_size)) {
                    return SND_ERR_CTX(SND_STATUS_PROTECTION_UPDATE_FAILED, "Section %.8s outside image bounds.",
                                       section->Name);
                }

                SIZE_T section_end = section_start + section_size;

                if (section_start < page_end && section_end > page_offset) {
                    if (section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
                        is_exec = TRUE;
                    if (section->Characteristics & IMAGE_SCN_MEM_READ)
                        is_read = TRUE;
                    if (section->Characteristics & IMAGE_SCN_MEM_WRITE)
                        is_write = TRUE;
                }
            }
        }

        DWORD characteristics = (is_exec ? IMAGE_SCN_MEM_EXECUTE : 0) | (is_read ? IMAGE_SCN_MEM_READ : 0) |
                                (is_write ? IMAGE_SCN_MEM_WRITE : 0);
        DWORD page_protect    = get_protection_flags(characteristics);

        if (page_offset == 0) {
            current_run_protect = page_protect;
            current_run_length  = SND_PAGE_SIZE;
        } else if (page_protect == current_run_protect) {
            current_run_length += SND_PAGE_SIZE;
        } else {
            void        *run_addr = SND_PTR_ADD(virtual_base, current_run_start);
            snd_status_t prot_status =
                mem_api->protect(run_addr, current_run_length, current_run_protect, &old_protect);
            if (prot_status.code != SND_SUCCESS) {
                return SND_ERR_CTX(SND_STATUS_PROTECTION_UPDATE_FAILED, "Failed to apply protection run");
            }
            current_run_start   = page_offset;
            current_run_protect = page_protect;
            current_run_length  = SND_PAGE_SIZE;
        }
    }

    if (current_run_length > 0) {
        void        *run_addr    = SND_PTR_ADD(virtual_base, current_run_start);
        snd_status_t prot_status = mem_api->protect(run_addr, current_run_length, current_run_protect, &old_protect);
        if (prot_status.code != SND_SUCCESS) {
            return SND_ERR_CTX(SND_STATUS_PROTECTION_UPDATE_FAILED, "Failed to apply final protection run");
        }
    }

    return SND_OK;
}

snd_status_t snd_allocate_and_copy_image(snd_loader_ctx_t *ctx) {
    if (ctx == NULL) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    if (ctx->stage != SND_STAGE_PARSED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE_SEQUENCE, "Sequence error: Expected %s, got %s",
                           stage_to_string(SND_STAGE_PARSED), stage_to_string(ctx->stage));
    }

    if (ctx->raw_source == NULL || ctx->raw_source->data == NULL) {
        return SND_ERR_CTX(SND_STATUS_CORRUPTED_STATE, "State is PARSED but raw_source data pointer is NULL.");
    }

    SIZE_T base = (SIZE_T)SND_PE_GET_NT_FIELD(&ctx->pe, OptionalHeader.ImageBase);
    SIZE_T size = SND_PE_GET_NT_FIELD(&ctx->pe, OptionalHeader.SizeOfImage);

    snd_status_t alloc_status =
        ctx->mem_api->alloc((LPVOID)base, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE, &ctx->target.virtual_base);
    if (alloc_status.code != SND_SUCCESS || ctx->target.virtual_base == NULL) {
        alloc_status =
            ctx->mem_api->alloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE, &ctx->target.virtual_base);
        if (alloc_status.code != SND_SUCCESS) {
            return alloc_status;
        }
    }

    ctx->target.allocated_size = size;
    ctx->stage                 = SND_STAGE_MEM_ALLOCATED;

    snd_status_t status = engine_pe_copy_sections(ctx->target.virtual_base, ctx->raw_source, &ctx->pe);
    if (status.code != SND_SUCCESS) {
        ctx->mem_api->free(ctx->target.virtual_base, 0, MEM_RELEASE);
        ctx->target.virtual_base   = NULL;
        ctx->target.allocated_size = 0;
        ctx->stage                 = SND_STAGE_PARSED;
        return status;
    }

    snd_buffer_t mapped_target_buf = {.data = ctx->target.virtual_base, .size = ctx->target.allocated_size};

    status = snd_pe_parse(&mapped_target_buf, TRUE, &ctx->pe);
    if (status.code != SND_SUCCESS) {
        ctx->mem_api->free(ctx->target.virtual_base, 0, MEM_RELEASE);
        ctx->target.virtual_base   = NULL;
        ctx->target.allocated_size = 0;
        if (ctx->raw_source != NULL && ctx->raw_source->data != NULL) {
            snd_pe_parse(ctx->raw_source, FALSE, &ctx->pe);
            ctx->stage = SND_STAGE_PARSED;
        } else {
            ctx->stage = SND_STAGE_UNINITIALIZED;
        }
        return status;
    }

    ctx->stage = SND_STAGE_SECTIONS_MAPPED;
    return SND_OK;
}

snd_status_t snd_apply_relocations(snd_loader_ctx_t *ctx) {
    if (ctx == NULL)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);

    if (ctx->stage != SND_STAGE_SECTIONS_MAPPED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE_SEQUENCE, "Sequence error: Expected %s, got %s",
                           stage_to_string(SND_STAGE_SECTIONS_MAPPED), stage_to_string(ctx->stage));
    }

    if (ctx->target.virtual_base == NULL) {
        return SND_ERR_CTX(SND_STATUS_CORRUPTED_STATE,
                           "State mismatch: Stage reports %s but physical "
                           "virtual_base pointer is NULL.",
                           stage_to_string(ctx->stage));
    }

    SIZE_T base = (SIZE_T)SND_PE_GET_NT_FIELD(&ctx->pe, OptionalHeader.ImageBase);

    ctx->target.delta_offset = (LONG_PTR)ctx->target.virtual_base - (LONG_PTR)base;

    snd_status_t status = snd_pe_apply_relocations(ctx->target.virtual_base, ctx->target.delta_offset, &ctx->pe);

    if (status.code != SND_SUCCESS) {
        return status;
    }

    ctx->stage = SND_STAGE_RELOCATED;

    return SND_OK;
}

snd_status_t snd_resolve_imports(snd_loader_ctx_t *ctx) {
    if (ctx == NULL)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);

    if (ctx->stage != SND_STAGE_RELOCATED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE_SEQUENCE, "Sequence error: Expected %s, got %s",
                           stage_to_string(SND_STAGE_RELOCATED), stage_to_string(ctx->stage));
    }

    if (ctx->target.virtual_base == NULL) {
        return SND_ERR_CTX(SND_STATUS_CORRUPTED_STATE, "State mismatch: Stage is %s but virtual_base is NULL.",
                           stage_to_string(SND_STAGE_RELOCATED));
    }

    snd_status_t status = snd_pe_resolve_imports(ctx->target.virtual_base, ctx->mod_api, &ctx->pe);

    if (status.code != SND_SUCCESS) {
        return status;
    }

    ctx->stage = SND_STAGE_IMPORTS_RESOLVED;

    return SND_OK;
}

snd_status_t snd_apply_memory_protections(snd_loader_ctx_t *ctx) {
    if (ctx == NULL)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);

    if (ctx->stage != SND_STAGE_IMPORTS_RESOLVED) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE_SEQUENCE, "Sequence error: Expected %s, got %s",
                           stage_to_string(SND_STAGE_IMPORTS_RESOLVED), stage_to_string(ctx->stage));
    }

    if (ctx->target.virtual_base == NULL) {
        return SND_ERR_CTX(SND_STATUS_CORRUPTED_STATE, "State mismatch: Stage is %s but virtual_base is NULL.",
                           stage_to_string(SND_STAGE_IMPORTS_RESOLVED));
    }

    snd_status_t status = engine_pe_apply_memory_protections(ctx->target.virtual_base, &ctx->pe, ctx->mem_api);
    if (status.code != SND_SUCCESS) {
        return status;
    }

    ctx->stage = SND_STAGE_READY_FOR_EXECUTION;

    return SND_OK;
}

snd_status_t snd_get_entry_point(snd_loader_ctx_t *ctx) {
    if (ctx == NULL) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    if (ctx->stage < SND_STAGE_READY_FOR_EXECUTION) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE_SEQUENCE, "Sequence error: Expected %s, got %s",
                           stage_to_string(SND_STAGE_READY_FOR_EXECUTION), stage_to_string(ctx->stage));
    }

    ctx->target.entry_point = snd_pe_get_entry_point(&ctx->pe);

    return SND_OK;
}

void snd_free_mapped_image(snd_loader_ctx_t *ctx) {
    if (ctx != NULL && ctx->target.virtual_base != NULL) {
        ctx->mem_api->free(ctx->target.virtual_base, 0, MEM_RELEASE);

        ctx->target.virtual_base   = NULL;
        ctx->target.allocated_size = 0;
        ctx->target.entry_point    = NULL;
        if (ctx->raw_source != NULL && ctx->raw_source->data != NULL) {
            snd_pe_parse(ctx->raw_source, FALSE, &ctx->pe);
            ctx->stage = SND_STAGE_PARSED;
        } else {
            ctx->stage = SND_STAGE_UNINITIALIZED;
        }
    }
}

snd_status_t snd_compatibility_check(snd_loader_ctx_t *ctx) {
    if (ctx == NULL) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    return snd_pe_compatibility_check(ctx->pe.is_64bit) ? SND_OK : SND_ERR(SND_STATUS_ARCH_MISMATCH);
}

void snd_execute_tls_callbacks(snd_loader_ctx_t *ctx, DWORD reason) {
    if (ctx == NULL || ctx->stage < SND_STAGE_READY_FOR_EXECUTION) {
        return;
    }

    snd_pe_execute_tls_callbacks(ctx->target.virtual_base, &ctx->pe, reason);
}

snd_status_t snd_get_proc_address(snd_loader_ctx_t *ctx, const char *func_name, FARPROC *func_addr_out) {
    if (ctx == NULL)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);

    if (ctx->stage < SND_STAGE_READY_FOR_EXECUTION) {
        return SND_ERR_CTX(SND_STATUS_INVALID_STAGE_SEQUENCE,
                           "Sequence error: Cannot resolve exports until image "
                           "is %s. Current state: %s",
                           stage_to_string(SND_STAGE_READY_FOR_EXECUTION), stage_to_string(ctx->stage));
    }

    return snd_pe_get_export_address(ctx->target.virtual_base, ctx->target.allocated_size, func_name, func_addr_out,
                                     ctx->mod_api->get_module_base);
}

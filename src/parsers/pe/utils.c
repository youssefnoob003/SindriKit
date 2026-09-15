#include <sindri/common/buffer.h>
#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/internal/nt/base.h>
#include <sindri/internal/nt/process.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/internal/windows/pe.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/parsers/pe/section.h>
#include <sindri/parsers/pe/status.h>
#include <sindri/parsers/pe/utils.h>

SND_FORCE_INLINE DWORD get_protection_flags(DWORD characteristics) {
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

snd_status_t snd_pe_get_directory(const snd_pe_parser_t *parser, DWORD index, SND_IMAGE_DATA_DIRECTORY *dir_out) {
    SND_CHECK_NULL(parser, dir_out);

    snd_memzero(dir_out, sizeof(SND_IMAGE_DATA_DIRECTORY));

    DWORD num_dirs = SND_PE_GET_NT_FIELD(parser, OptionalHeader.NumberOfRvaAndSizes);
    if (index >= num_dirs || index >= SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES) {
        return SND_ERR(SND_STATUS_DIRECTORY_ENTRY_MISSING);
    }

    SIZE_T nt_header_size = parser->is_64bit ? offsetof(SND_IMAGE_NT_HEADERS64, OptionalHeader)
                                             : offsetof(SND_IMAGE_NT_HEADERS32, OptionalHeader);

    SIZE_T dir_offset = parser->is_64bit ? offsetof(SND_IMAGE_OPTIONAL_HEADER64, DataDirectory)
                                         : offsetof(SND_IMAGE_OPTIONAL_HEADER32, DataDirectory);

    SIZE_T total_offset = nt_header_size + dir_offset + (index + 1) * sizeof(SND_IMAGE_DATA_DIRECTORY);

    if (SND_RANGE_EXCEEDS(parser->lfanew, total_offset, parser->source.size)) {
        return SND_ERR(SND_STATUS_DIRECTORY_ENTRY_INVALID);
    }

    SND_IMAGE_DATA_DIRECTORY dir_tmp = SND_PE_GET_NT_FIELD(parser, OptionalHeader.DataDirectory[index]);

    if (dir_tmp.VirtualAddress == 0) {
        return SND_ERR(SND_STATUS_DIRECTORY_ENTRY_MISSING);
    }

    *dir_out = dir_tmp;

    return SND_OK;
}

PVOID snd_pe_rva_to_ptr(const snd_pe_parser_t *parser, DWORD rva, SIZE_T size) {
    if (!parser || !parser->source.data) {
        return NULL;
    }

    if (parser->is_mapped) {
        if (!snd_buffer_bounds_check(&parser->source, rva, size)) {
            return NULL;
        }
        return SND_PTR_ADD(parser->source.data, rva);
    }

    DWORD size_of_headers = SND_PE_GET_NT_FIELD(parser, OptionalHeader.SizeOfHeaders);

    if ((SIZE_T)size_of_headers > parser->source.size) {
        size_of_headers = (DWORD)parser->source.size;
    }

    if (parser->sections_count > 0 && parser->section_head != NULL) {
        DWORD min_raw_offset = size_of_headers;
        for (DWORD i = 0; i < parser->sections_count; i++) {
            DWORD raw_ptr = parser->section_head[i].PointerToRawData;
            if (raw_ptr > 0 && raw_ptr < min_raw_offset) {
                min_raw_offset = raw_ptr;
            }
        }
        size_of_headers = min_raw_offset;
    }

    if (rva < size_of_headers) {
        if (SND_RANGE_EXCEEDS(rva, size, size_of_headers)) {
            return NULL;
        }
        if (!snd_buffer_bounds_check(&parser->source, rva, size)) {
            return NULL;
        }
        return SND_PTR_ADD(parser->source.data, rva);
    }

    DWORD section_alignment = SND_PE_GET_NT_FIELD(parser, OptionalHeader.SectionAlignment);
    DWORD file_alignment    = SND_PE_GET_NT_FIELD(parser, OptionalHeader.FileAlignment);

    for (int i = (int)parser->sections_count - 1; i >= 0; i--) {
        PSND_IMAGE_SECTION_HEADER section    = &parser->section_head[i];
        DWORD                     aligned_va = section->VirtualAddress;
        if (section_alignment >= SND_PAGE_SIZE) {
            aligned_va = SND_ALIGN_DOWN(aligned_va, section_alignment);
        }

        DWORD virtual_size = section->Misc.VirtualSize;
        if (virtual_size == 0) {
            virtual_size = section->SizeOfRawData;
        }

        DWORD aligned_raw_size = section->SizeOfRawData;
        if (file_alignment != 0) {
            if (SND_ADD_OVERFLOWS_DWORD((file_alignment - 1), section->SizeOfRawData)) {
                return NULL;
            }
            aligned_raw_size = SND_ALIGN_UP(section->SizeOfRawData, file_alignment);
        }

        DWORD max_size = SND_MAX(virtual_size, aligned_raw_size);

        if (SND_IN_BOUNDS(rva, aligned_va, max_size)) {
            DWORD offset_in_section = rva - aligned_va;

            if (offset_in_section >= aligned_raw_size) {
                return NULL;
            }

            if (SND_RANGE_EXCEEDS(offset_in_section, size, aligned_raw_size)) {
                return NULL;
            }

            DWORD aligned_ptr_raw = section->PointerToRawData;
            if (file_alignment >= SND_PE_MIN_FILE_ALIGNMENT) {
                aligned_ptr_raw = SND_ALIGN_DOWN(aligned_ptr_raw, file_alignment);
            }

            if (SND_ADD_OVERFLOWS_DWORD(aligned_ptr_raw, offset_in_section)) {
                continue;
            }

            DWORD foa = aligned_ptr_raw + offset_in_section;
            if (!snd_buffer_bounds_check(&parser->source, foa, size)) {
                return NULL;
            }
            return SND_PTR_ADD(parser->source.data, foa);
        }
    }
    return NULL;
}

PVOID snd_pe_get_entry_point(const snd_pe_parser_t *parser) {
    if (!parser) {
        return NULL;
    }

    DWORD ep_rva = SND_PE_GET_NT_FIELD(parser, OptionalHeader.AddressOfEntryPoint);
    if (ep_rva == 0) {
        return NULL;
    }

    return snd_pe_rva_to_ptr(parser, ep_rva, 1);
}

static BOOL pe_va_to_rva(const snd_pe_parser_t *parser, ULONGLONG va, DWORD *rva_out) {
    if (!parser || !rva_out || va == 0)
        return FALSE;

    ULONGLONG base = parser->is_mapped ? (ULONGLONG)(ULONG_PTR)parser->source.data
                                       : (ULONGLONG)SND_PE_GET_NT_FIELD(parser, OptionalHeader.ImageBase);

    if (va < base)
        return FALSE;

    ULONGLONG rva = va - base;
    if (rva > SND_MAX_DWORD)
        return FALSE;

    *rva_out = (DWORD)rva;
    return TRUE;
}

PVOID snd_pe_get_tls_callbacks(const snd_pe_parser_t *parser) {
    if (!parser || !parser->source.data)
        return NULL;

    SND_IMAGE_DATA_DIRECTORY tls_dir = {0};
    if (SND_FAILED(snd_pe_get_directory(parser, SND_IMAGE_DIRECTORY_ENTRY_TLS, &tls_dir))) {
        return NULL;
    }

    ULONGLONG callbacks_va = 0;
    if (parser->is_64bit) {
        PSND_IMAGE_TLS_DIRECTORY64 tls64 = (PSND_IMAGE_TLS_DIRECTORY64)snd_pe_rva_to_ptr(
            parser, tls_dir.VirtualAddress, sizeof(SND_IMAGE_TLS_DIRECTORY64));
        if (!tls64)
            return NULL;
        callbacks_va = tls64->AddressOfCallBacks;
    } else {
        PSND_IMAGE_TLS_DIRECTORY32 tls32 = (PSND_IMAGE_TLS_DIRECTORY32)snd_pe_rva_to_ptr(
            parser, tls_dir.VirtualAddress, sizeof(SND_IMAGE_TLS_DIRECTORY32));
        if (!tls32)
            return NULL;
        callbacks_va = (ULONGLONG)tls32->AddressOfCallBacks;
    }

    DWORD callbacks_rva = 0;
    if (!pe_va_to_rva(parser, callbacks_va, &callbacks_rva))
        return NULL;

    SIZE_T entry_size = parser->is_64bit ? sizeof(ULONGLONG) : sizeof(DWORD);
    return snd_pe_rva_to_ptr(parser, callbacks_rva, entry_size);
}

DWORD snd_pe_get_page_protection_flags(const snd_pe_parser_t *pe, SIZE_T page_offset) {
    DWORD flags        = 0;
    DWORD headers_size = SND_PE_GET_NT_FIELD(pe, OptionalHeader.SizeOfHeaders);

    if (page_offset < headers_size) {
        flags |= SND_IMAGE_SCN_MEM_READ;
    }

    SIZE_T page_end =
        SND_ADD_OVERFLOWS_SIZET(page_offset, SND_PAGE_SIZE) ? SND_MAX_SIZE_T : page_offset + SND_PAGE_SIZE;

    for (DWORD i = 0; i < pe->sections_count; i++) {
        PSND_IMAGE_SECTION_HEADER section     = &pe->section_head[i];
        SIZE_T                    loaded_size = (SIZE_T)snd_pe_section_loaded_size(section);
        if (loaded_size == 0)
            continue;

        SIZE_T section_start = (SIZE_T)section->VirtualAddress;
        SIZE_T section_end =
            SND_ADD_OVERFLOWS_SIZET(section_start, loaded_size) ? SND_MAX_SIZE_T : section_start + loaded_size;

        if (section_start < page_end && section_end > page_offset) {
            flags |= (section->Characteristics &
                      (SND_IMAGE_SCN_MEM_EXECUTE | SND_IMAGE_SCN_MEM_READ | SND_IMAGE_SCN_MEM_WRITE));
        }
    }

    return get_protection_flags(flags);
}

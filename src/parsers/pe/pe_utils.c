#include <sindri/common/helpers.h>
#include <sindri/common/status.h>
#include <sindri/internal/nt_defs.h>
#include <sindri/parsers/pe/pe_parser.h>
#include <sindri/parsers/pe/pe_utils.h>
#include <windows.h>

snd_status_t snd_pe_get_directory(const snd_pe_parser_t *parser, DWORD index, IMAGE_DATA_DIRECTORY *dir_out) {
    if (!parser || !dir_out)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    snd_memzero(dir_out, sizeof(IMAGE_DATA_DIRECTORY));

    DWORD num_dirs = SND_PE_GET_NT_FIELD(parser, OptionalHeader.NumberOfRvaAndSizes);
    if (index >= num_dirs || index >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES) {
        return SND_ERR_CTX(SND_STATUS_DIRECTORY_NOT_FOUND, "Directory index out of bounds");
    }

    SIZE_T dir_offset = parser->is_64bit ? offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory)
                                         : offsetof(IMAGE_OPTIONAL_HEADER32, DataDirectory);
    SIZE_T nt_off     = (SIZE_T)parser->dos->e_lfanew;

    SIZE_T total_offset =
        offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + dir_offset + (index + 1) * sizeof(IMAGE_DATA_DIRECTORY);
    if (total_offset > parser->source.size || nt_off > parser->source.size - total_offset) {
        return SND_ERR_CTX(SND_STATUS_DIRECTORY_NOT_FOUND, "Directory offset out of bounds");
    }

    if (parser->is_64bit) {
        *dir_out = parser->nt.nt64->OptionalHeader.DataDirectory[index];
    } else {
        *dir_out = parser->nt.nt32->OptionalHeader.DataDirectory[index];
    }
    return SND_OK;
}

PVOID snd_pe_rva_to_ptr(const snd_pe_parser_t *parser, DWORD rva, SIZE_T size) {
    if (!parser || !parser->source.data || rva == 0)
        return NULL;

    if (parser->is_mapped) {
        if (!snd_buffer_bounds_check(&parser->source, rva, size)) {
            return NULL;
        }
        return SND_PTR_ADD(parser->source.data, rva);
    }

    DWORD size_of_headers = SND_PE_GET_NT_FIELD(parser, OptionalHeader.SizeOfHeaders);
    if (rva < size_of_headers) {
        if (rva + size > size_of_headers)
            return NULL;
        if (!snd_buffer_bounds_check(&parser->source, rva, size))
            return NULL;
        return SND_PTR_ADD(parser->source.data, rva);
    }

    DWORD section_alignment = SND_PE_GET_NT_FIELD(parser, OptionalHeader.SectionAlignment);
    DWORD file_alignment    = SND_PE_GET_NT_FIELD(parser, OptionalHeader.FileAlignment);

    for (DWORD i = parser->sections_count - 1; i >= 0; i--) {
        PIMAGE_SECTION_HEADER section    = &parser->section_head[i];
        DWORD                 aligned_va = section->VirtualAddress;
        if (section_alignment >= SND_PAGE_SIZE) {
            aligned_va = (aligned_va / section_alignment) * section_alignment;
        }

        DWORD virtual_size = section->Misc.VirtualSize;
        if (virtual_size == 0)
            virtual_size = section->SizeOfRawData;

        DWORD aligned_raw_size = section->SizeOfRawData;
        if (file_alignment != 0) {
            aligned_raw_size = ((aligned_raw_size + file_alignment - 1) / file_alignment) * file_alignment;
        }

        DWORD max_size = virtual_size > aligned_raw_size ? virtual_size : aligned_raw_size;

        if (rva >= aligned_va && rva - aligned_va < max_size) {
            DWORD offset_in_section = rva - aligned_va;

            if (offset_in_section >= aligned_raw_size) {
                return NULL;
            }

            if (size > (SIZE_T)(aligned_raw_size - offset_in_section)) {
                return NULL;
            }

            DWORD aligned_ptr_raw = section->PointerToRawData;
            if (file_alignment >= SND_PE_MIN_FILE_ALIGNMENT) {
                aligned_ptr_raw = (aligned_ptr_raw / file_alignment) * file_alignment;
            }

            if (MAXDWORD - aligned_ptr_raw < offset_in_section) {
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

BOOL snd_pe_compatibility_check(BOOL is_64bit) {
#if defined(_WIN64)
    if (!is_64bit)
        return FALSE;
#elif defined(_WIN32)
    if (is_64bit)
        return FALSE;
#else
#error "Unsupported architecture: SindriKit requires _WIN32 or _WIN64"
#endif
    return TRUE;
}

void *snd_pe_get_entry_point(const snd_pe_parser_t *parser) {
    if (!parser)
        return NULL;

    DWORD ep_rva = SND_PE_GET_NT_FIELD(parser, OptionalHeader.AddressOfEntryPoint);
    if (ep_rva == 0)
        return NULL;

    return snd_pe_rva_to_ptr(parser, ep_rva, 1);
}

PVOID snd_pe_get_tls_callbacks(PVOID base_address, const snd_pe_parser_t *parser) {
    if (!base_address || !parser)
        return NULL;

    IMAGE_DATA_DIRECTORY tls_dir = {0};
    snd_pe_get_directory(parser, IMAGE_DIRECTORY_ENTRY_TLS, &tls_dir);

    if (tls_dir.VirtualAddress == 0)
        return NULL;

    ULONG_PTR callbacks_va = 0;

    if (parser->is_64bit) {
        PIMAGE_TLS_DIRECTORY64 tls_struct64 =
            (PIMAGE_TLS_DIRECTORY64)snd_pe_rva_to_ptr(parser, tls_dir.VirtualAddress, sizeof(IMAGE_TLS_DIRECTORY64));
        if (!tls_struct64)
            return NULL;
        callbacks_va = (ULONG_PTR)tls_struct64->AddressOfCallBacks;
    } else {
        PIMAGE_TLS_DIRECTORY32 tls_struct32 =
            (PIMAGE_TLS_DIRECTORY32)snd_pe_rva_to_ptr(parser, tls_dir.VirtualAddress, sizeof(IMAGE_TLS_DIRECTORY32));
        if (!tls_struct32)
            return NULL;
        callbacks_va = (ULONG_PTR)tls_struct32->AddressOfCallBacks;
    }

    if (callbacks_va == 0)
        return NULL;

    ULONG_PTR base_to_subtract = (ULONG_PTR)SND_PE_GET_NT_FIELD(parser, OptionalHeader.ImageBase);
    if (parser->is_mapped) {
        base_to_subtract = (ULONG_PTR)base_address;
    }

    if (callbacks_va < base_to_subtract)
        return NULL;

    ULONG_PTR callbacks_rva = callbacks_va - base_to_subtract;
    if (callbacks_rva > 0xFFFFFFFF) {
        return NULL;
    }

    return snd_pe_rva_to_ptr(parser, (DWORD)callbacks_rva, parser->is_64bit ? sizeof(ULONGLONG) : sizeof(DWORD));
}

snd_status_t snd_pe_execute_tls_callbacks(PVOID virtual_base, const snd_pe_parser_t *parser, DWORD reason) {
    if (!virtual_base || !parser)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);

    if (!parser->is_mapped) {
        return SND_ERR_CTX(SND_STATUS_UNSUPPORTED, "Executing TLS callbacks from an unmapped raw file is forbidden.");
    }

    BOOL arch_check = snd_pe_compatibility_check(parser->is_64bit);
    if (!arch_check) {
        return SND_ERR(SND_STATUS_ARCH_MISMATCH);
    }

    PVOID callbacks_ptr = snd_pe_get_tls_callbacks(virtual_base, parser);
    if (callbacks_ptr == NULL)
        return SND_OK;

    SIZE_T callback_stride = parser->is_64bit ? sizeof(ULONGLONG) : sizeof(DWORD);
    SIZE_T offset          = 0;
    SIZE_T base_rva        = (SIZE_T)((BYTE *)callbacks_ptr - (BYTE *)virtual_base);

    while (1) {
        PVOID cb_ptr = snd_pe_rva_to_ptr(parser, (DWORD)(base_rva + offset), callback_stride);
        if (!cb_ptr)
            break;

        PIMAGE_TLS_CALLBACK cb = NULL;

        if (parser->is_64bit) {
            ULONGLONG cb_addr = *(ULONGLONG *)cb_ptr;
            if (cb_addr == 0)
                break;

            SIZE_T cb_rva = (SIZE_T)cb_addr - (SIZE_T)virtual_base;
            if (!snd_pe_rva_to_ptr(parser, (DWORD)cb_rva, 1)) {
                break;
            }
            cb = (PIMAGE_TLS_CALLBACK)(ULONG_PTR)cb_addr;
        } else {
            DWORD cb_addr = *(DWORD *)cb_ptr;
            if (cb_addr == 0)
                break;

            SIZE_T cb_rva = (SIZE_T)cb_addr - (SIZE_T)virtual_base;
            if (!snd_pe_rva_to_ptr(parser, (DWORD)cb_rva, 1)) {
                break;
            }
            cb = (PIMAGE_TLS_CALLBACK)(ULONG_PTR)cb_addr;
        }

        cb(virtual_base, reason, NULL);

        offset += callback_stride;
    }

    return SND_OK;
}

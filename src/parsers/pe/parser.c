#include <sindri/common/buffer.h>
#include <sindri/common/memory.h>
#include <sindri/internal/windows/pe.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/parsers/pe/status.h>
#include <sindri/parsers/pe/utils.h>

snd_status_t snd_pe_parse(const snd_buffer_t *source, BOOL is_mapped, snd_pe_parser_t *parser) {
    SND_CHECK_NULL(source, source->data, source->size, parser);

    snd_memzero(parser, sizeof(snd_pe_parser_t));
    parser->source    = *source;
    parser->is_mapped = is_mapped;

    if (!snd_buffer_bounds_check(source, 0, sizeof(SND_IMAGE_DOS_HEADER))) {
        return SND_ERR_CTX(SND_STATUS_HEADER_DOS_TRUNCATED, "Buffer size (%zu) smaller than SND_IMAGE_DOS_HEADER (%zu)",
                           source->size, sizeof(SND_IMAGE_DOS_HEADER));
    }

    parser->dos = (PSND_IMAGE_DOS_HEADER)source->data;
    if (parser->dos->e_magic != SND_IMAGE_DOS_SIGNATURE) {
        return SND_ERR_CTX(SND_STATUS_HEADER_DOS_SIGNATURE_INVALID,
                           "e_magic 0x%04X != SND_IMAGE_DOS_SIGNATURE (0x%04X)", parser->dos->e_magic,
                           SND_IMAGE_DOS_SIGNATURE);
    }

    LONG raw_lfanew = parser->dos->e_lfanew;
    if (raw_lfanew < 0) {
        return SND_ERR_CTX(SND_STATUS_HEADER_OFFSET_INVALID, "e_lfanew (%ld) cannot be negative", raw_lfanew);
    }

    parser->lfanew        = (SIZE_T)raw_lfanew;
    SIZE_T req_magic_size = sizeof(DWORD) + sizeof(SND_IMAGE_FILE_HEADER) + sizeof(WORD);

    if (!snd_buffer_bounds_check(source, parser->lfanew, req_magic_size)) {
        return SND_ERR_CTX(SND_STATUS_HEADER_NT_TRUNCATED, "e_lfanew 0x%zx + core headers exceeds buffer size 0x%zx",
                           parser->lfanew, source->size);
    }

    PSND_IMAGE_NT_HEADERS32 nt_chk = (PSND_IMAGE_NT_HEADERS32)SND_PTR_ADD(source->data, parser->lfanew);
    if (nt_chk->Signature != SND_IMAGE_NT_SIGNATURE) {
        return SND_ERR_CTX(SND_STATUS_HEADER_NT_SIGNATURE_INVALID,
                           "NT signature 0x%08X != SND_IMAGE_NT_SIGNATURE (0x%08X)", nt_chk->Signature,
                           SND_IMAGE_NT_SIGNATURE);
    }

    WORD   optional_magic   = nt_chk->OptionalHeader.Magic;
    SIZE_T required_nt_size = sizeof(DWORD) + sizeof(SND_IMAGE_FILE_HEADER);

    if (optional_magic == SND_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        required_nt_size += offsetof(SND_IMAGE_OPTIONAL_HEADER64, DataDirectory);

        if (!snd_buffer_bounds_check(source, parser->lfanew, required_nt_size)) {
            return SND_ERR_CTX(SND_STATUS_HEADER_NT_TRUNCATED, "Buffer size truncated before 64-bit NT header");
        }
        parser->is_64bit = TRUE;
        parser->nt.nt64  = (PSND_IMAGE_NT_HEADERS64)nt_chk;
    } else if (optional_magic == SND_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        required_nt_size += offsetof(SND_IMAGE_OPTIONAL_HEADER32, DataDirectory);

        if (!snd_buffer_bounds_check(source, parser->lfanew, required_nt_size)) {
            return SND_ERR_CTX(SND_STATUS_HEADER_NT_TRUNCATED, "Buffer size truncated before 32-bit NT header");
        }
        parser->is_64bit = FALSE;
        parser->nt.nt32  = nt_chk;
    } else {
        return SND_ERR_CTX(SND_STATUS_HEADER_OPTIONAL_SIGNATURE_INVALID,
                           "OptionalHeader.Magic 0x%04X not PE32 (0x010B) or PE32+ (0x020B)", optional_magic);
    }

    parser->is_dll         = (SND_PE_GET_NT_FIELD(parser, FileHeader.Characteristics) & SND_IMAGE_FILE_DLL) != 0;
    parser->sections_count = SND_PE_GET_NT_FIELD(parser, FileHeader.NumberOfSections);

    SIZE_T fixed_headers_size = offsetof(SND_IMAGE_NT_HEADERS32, OptionalHeader);
    WORD   opt_header_size    = SND_PE_GET_NT_FIELD(parser, FileHeader.SizeOfOptionalHeader);
    SIZE_T total_header_size  = fixed_headers_size + opt_header_size;

    if (parser->is_mapped && parser->source.size == SND_SYS_DLL_SIZE_DEFAULT) {
        SIZE_T image_size = SND_PE_GET_NT_FIELD(parser, OptionalHeader.SizeOfImage);
        if (image_size > 0) {
            parser->source.size = image_size;
        }
    }

    if (SND_RANGE_EXCEEDS(parser->lfanew, total_header_size, parser->source.size)) {
        parser->sections_count = 0;
        parser->section_head   = NULL;
    } else {
        SIZE_T section_table_off = parser->lfanew + total_header_size;
        SIZE_T available_space   = parser->source.size - section_table_off;
        DWORD  max_sections      = (DWORD)(available_space / sizeof(SND_IMAGE_SECTION_HEADER));

        if (parser->sections_count > max_sections) {
            parser->sections_count = max_sections;
        }
        parser->section_head = (PSND_IMAGE_SECTION_HEADER)SND_PTR_ADD(parser->source.data, section_table_off);
    }

    parser->string_table = NULL;

    if (!is_mapped) {
        DWORD symbol_table_off = SND_PE_GET_NT_FIELD(parser, FileHeader.PointerToSymbolTable);
        DWORD symbol_count     = SND_PE_GET_NT_FIELD(parser, FileHeader.NumberOfSymbols);

        if (symbol_table_off != 0 && symbol_count != 0) {
            ULONGLONG string_table_off =
                (ULONGLONG)symbol_table_off + ((ULONGLONG)SND_IMAGE_SIZEOF_SYMBOL * (ULONGLONG)symbol_count);

            if (string_table_off <= (ULONGLONG)SIZE_MAX &&
                snd_buffer_bounds_check(&parser->source, (SIZE_T)string_table_off, sizeof(DWORD))) {
                parser->string_table = SND_PTR_ADD(parser->source.data, (SIZE_T)string_table_off);
            }
        }
    }

    SND_IMAGE_DATA_DIRECTORY import_dir = {0};
    snd_pe_get_directory(parser, SND_IMAGE_DIRECTORY_ENTRY_IMPORT, &import_dir);
    parser->imports_rva = import_dir.VirtualAddress;
    parser->import_size = import_dir.Size;

    if (parser->imports_rva == 0) {
        parser->import_size = 0;
    }

    return SND_OK;
}

#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/parsers/pe/utils.h>
#include <stddef.h>
#include <windows.h>

snd_status_t snd_pe_parse(const snd_buffer_t *buf, BOOL is_mapped, snd_pe_parser_t *parser) {
    if (buf == NULL || buf->data == NULL || buf->size == 0 || parser == NULL) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    snd_memzero(parser, sizeof(snd_pe_parser_t));

    parser->source    = *buf;
    parser->is_mapped = is_mapped;

    if (!snd_buffer_bounds_check(buf, 0, sizeof(IMAGE_DOS_HEADER))) {
        return SND_ERR(SND_STATUS_DOS_HEADER_TRUNCATED);
    }

    parser->dos = (PIMAGE_DOS_HEADER)buf->data;
    if (parser->dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return SND_ERR(SND_STATUS_INVALID_DOS_SIGNATURE);
    }

    if (parser->dos->e_lfanew < 0) {
        return SND_ERR(SND_STATUS_INVALID_NT_HEADER_OFFSET);
    }

    SIZE_T nt_off         = (SIZE_T)parser->dos->e_lfanew;
    SIZE_T req_magic_size = sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + sizeof(WORD);
    if (!snd_buffer_bounds_check(buf, nt_off, req_magic_size)) {
        return SND_ERR(SND_STATUS_NT_HEADERS_TRUNCATED);
    }

    PIMAGE_NT_HEADERS32 nt_chk = (PIMAGE_NT_HEADERS32)SND_PTR_ADD(buf->data, nt_off);
    if (nt_chk->Signature != IMAGE_NT_SIGNATURE) {
        return SND_ERR(SND_STATUS_INVALID_NT_SIGNATURE);
    }

    WORD   optional_magic   = nt_chk->OptionalHeader.Magic;
    SIZE_T required_nt_size = sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);

    if (optional_magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        required_nt_size += offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory);
        if (!snd_buffer_bounds_check(buf, nt_off, required_nt_size)) {
            return SND_ERR(SND_STATUS_NT_HEADERS_TRUNCATED);
        }
        parser->is_64bit       = TRUE;
        parser->nt.nt64        = (PIMAGE_NT_HEADERS64)nt_chk;
        parser->is_dll         = (parser->nt.nt64->FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;
        parser->sections_count = parser->nt.nt64->FileHeader.NumberOfSections;
    } else if (optional_magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        required_nt_size += offsetof(IMAGE_OPTIONAL_HEADER32, DataDirectory);
        if (!snd_buffer_bounds_check(buf, nt_off, required_nt_size)) {
            return SND_ERR(SND_STATUS_NT_HEADERS_TRUNCATED);
        }
        parser->is_64bit       = FALSE;
        parser->nt.nt32        = nt_chk;
        parser->is_dll         = (parser->nt.nt32->FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;
        parser->sections_count = parser->nt.nt32->FileHeader.NumberOfSections;
    } else {
        return SND_ERR(SND_STATUS_UNSUPPORTED_OPTIONAL_HEADER_MAGIC);
    }

    SIZE_T fixed_headers_size = offsetof(IMAGE_NT_HEADERS32, OptionalHeader);
    WORD   opt_header_size    = SND_PE_GET_NT_FIELD(parser, FileHeader.SizeOfOptionalHeader);

    if (fixed_headers_size + opt_header_size > buf->size ||
        nt_off > buf->size - (fixed_headers_size + opt_header_size)) {
        parser->sections_count = 0;
        parser->section_head   = NULL;
    } else {
        SIZE_T section_table_off = nt_off + fixed_headers_size + opt_header_size;
        SIZE_T available_space   = buf->size - section_table_off;
        DWORD  max_sections      = (DWORD)(available_space / sizeof(IMAGE_SECTION_HEADER));
        if (parser->sections_count > max_sections) {
            parser->sections_count = max_sections;
        }
        parser->section_head = (PIMAGE_SECTION_HEADER)SND_PTR_ADD(buf->data, section_table_off);
    }

    if (!is_mapped) {
        DWORD symbol_table_off = SND_PE_GET_NT_FIELD(parser, FileHeader.PointerToSymbolTable);
        DWORD symbol_count     = SND_PE_GET_NT_FIELD(parser, FileHeader.NumberOfSymbols);
        if (symbol_table_off != 0 && symbol_count != 0) {
            ULONGLONG string_table_off =
                (ULONGLONG)symbol_table_off + (ULONGLONG)IMAGE_SIZEOF_SYMBOL * (ULONGLONG)symbol_count;
            if (snd_memory_bounds_check(buf->size, (SIZE_T)string_table_off, sizeof(DWORD))) {
                parser->string_table = SND_PTR_ADD(buf->data, (SIZE_T)string_table_off);
            }
        }
    }

    IMAGE_DATA_DIRECTORY import_dir = {0};
    snd_pe_get_directory(parser, IMAGE_DIRECTORY_ENTRY_IMPORT, &import_dir);
    parser->imports_rva = import_dir.VirtualAddress;
    parser->import_size = import_dir.Size;

    if (parser->imports_rva == 0) {
        parser->import_size = 0;
        return SND_OK;
    }

    return SND_OK;
}

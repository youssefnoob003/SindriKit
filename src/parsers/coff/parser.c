#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/common/status.h>
#include <sindri/parsers/coff/parser.h>
#include <windows.h>

snd_status_t snd_coff_parse(const snd_buffer_t *buf, snd_coff_parser_t *parser) {
    if (buf == NULL || buf->data == NULL || buf->size == 0 || parser == NULL) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    snd_memzero(parser, sizeof(snd_coff_parser_t));
    parser->source = *buf;

    if (!snd_buffer_bounds_check(buf, 0, sizeof(IMAGE_FILE_HEADER))) {
        return SND_ERR(SND_STATUS_NT_HEADERS_TRUNCATED);
    }

    parser->file_header = (PIMAGE_FILE_HEADER)buf->data;

    if (parser->file_header->Machine == IMAGE_FILE_MACHINE_AMD64) {
        parser->is_64bit = TRUE;
    } else if (parser->file_header->Machine == IMAGE_FILE_MACHINE_I386) {
        parser->is_64bit = FALSE;
    } else {
        return SND_ERR(SND_STATUS_UNSUPPORTED);
    }

    parser->sections_count = parser->file_header->NumberOfSections;

    SIZE_T sections_off = sizeof(IMAGE_FILE_HEADER) + parser->file_header->SizeOfOptionalHeader;

    if (parser->sections_count > 0) {
        SIZE_T sections_size = (SIZE_T)parser->sections_count * sizeof(IMAGE_SECTION_HEADER);

        if (sections_off + sections_size < sections_off) {
            return SND_ERR(SND_STATUS_INTEGER_OVERFLOW);
        }

        if (!snd_buffer_bounds_check(buf, sections_off, sections_size)) {
            return SND_ERR(SND_STATUS_NT_HEADERS_TRUNCATED);
        }
        parser->section_head = (PIMAGE_SECTION_HEADER)SND_PTR_ADD(buf->data, sections_off);
    }

    DWORD symbol_table_off = parser->file_header->PointerToSymbolTable;
    parser->symbol_count   = parser->file_header->NumberOfSymbols;

    if (symbol_table_off != 0 && parser->symbol_count > 0) {
        if (parser->symbol_count > ((SIZE_T)-1) / sizeof(IMAGE_SYMBOL)) {
            return SND_ERR(SND_STATUS_INTEGER_OVERFLOW);
        }

        SIZE_T symbols_size = (SIZE_T)parser->symbol_count * sizeof(IMAGE_SYMBOL);
        if (!snd_buffer_bounds_check(buf, symbol_table_off, symbols_size)) {
            return SND_ERR(SND_STATUS_NT_HEADERS_TRUNCATED);
        }
        parser->symbol_table = (PIMAGE_SYMBOL)SND_PTR_ADD(buf->data, symbol_table_off);

        SIZE_T string_table_off = (SIZE_T)symbol_table_off + symbols_size;

        if (string_table_off < symbol_table_off) {
            return SND_ERR(SND_STATUS_INTEGER_OVERFLOW);
        }

        if (snd_buffer_bounds_check(buf, string_table_off, sizeof(DWORD))) {
            parser->string_table      = SND_PTR_ADD(buf->data, string_table_off);
            parser->string_table_size = *(DWORD *)parser->string_table;

            if (parser->string_table_size < sizeof(DWORD) ||
                !snd_buffer_bounds_check(buf, string_table_off, parser->string_table_size)) {
                parser->string_table      = NULL;
                parser->string_table_size = 0;
            }
        }
    }

    return SND_OK;
}

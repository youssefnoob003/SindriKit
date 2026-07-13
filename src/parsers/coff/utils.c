#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/coff/parser.h>
#include <sindri/parsers/coff/utils.h>
#include <windows.h>

snd_status_t snd_coff_get_symbol_name(const snd_coff_parser_t *parser, PIMAGE_SYMBOL symbol, char *name_out,
                                      SIZE_T name_len) {
    if (!parser || !symbol || !name_out || name_len == 0) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    snd_memzero(name_out, name_len);

    if (symbol->N.Name.Short == 0) {
        DWORD offset = symbol->N.Name.Long;

        if (parser->string_table == NULL || offset < sizeof(DWORD) || offset >= parser->string_table_size) {
            return SND_ERR(SND_STATUS_COFF_SYMBOL_NOT_FOUND);
        }

        char *long_name = (char *)(parser->string_table + offset);

        SIZE_T remaining = parser->string_table_size - offset;
        SIZE_T copy_len  = remaining < name_len - 1 ? remaining : name_len - 1;

        for (SIZE_T i = 0; i < copy_len; i++) {
            if (long_name[i] == '\0')
                break;
            name_out[i] = long_name[i];
        }
    } else {
        SIZE_T copy_len = 8 < name_len - 1 ? 8 : name_len - 1;
        for (SIZE_T i = 0; i < copy_len; i++) {
            if (symbol->N.ShortName[i] == '\0')
                break;
            name_out[i] = symbol->N.ShortName[i];
        }
    }

    return SND_OK;
}

snd_status_t snd_coff_get_section_name(const snd_coff_parser_t *parser, PIMAGE_SECTION_HEADER section, char *name_out,
                                       SIZE_T name_len) {
    if (!parser || !section || !name_out || name_len == 0) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    snd_memzero(name_out, name_len);

    if (section->Name[0] == '/') {
        DWORD offset = 0;
        for (int i = 1; i < 8; i++) {
            if (section->Name[i] >= '0' && section->Name[i] <= '9') {
                offset = (offset * 10) + (section->Name[i] - '0');
            } else {
                break;
            }
        }

        if (parser->string_table == NULL || offset < sizeof(DWORD) || offset >= parser->string_table_size) {
            return SND_ERR(SND_STATUS_COFF_SECTION_NOT_FOUND);
        }

        char  *long_name = (char *)(parser->string_table + offset);
        SIZE_T remaining = parser->string_table_size - offset;
        SIZE_T copy_len  = remaining < name_len - 1 ? remaining : name_len - 1;

        for (SIZE_T i = 0; i < copy_len; i++) {
            if (long_name[i] == '\0')
                break;
            name_out[i] = long_name[i];
        }
    } else {
        SIZE_T copy_len = 8 < name_len - 1 ? 8 : name_len - 1;
        for (SIZE_T i = 0; i < copy_len; i++) {
            if (section->Name[i] == '\0')
                break;
            name_out[i] = section->Name[i];
        }
    }

    return SND_OK;
}

PVOID snd_coff_raw_to_ptr(const snd_coff_parser_t *parser, DWORD raw_offset, SIZE_T size) {
    if (!parser || !parser->source.data || raw_offset == 0)
        return NULL;

    if (!snd_buffer_bounds_check(&parser->source, raw_offset, size)) {
        return NULL;
    }

    return SND_PTR_ADD(parser->source.data, raw_offset);
}

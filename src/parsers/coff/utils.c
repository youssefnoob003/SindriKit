#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/common/string.h>
#include <sindri/internal/windows/coff.h>
#include <sindri/parsers/coff/parser.h>
#include <sindri/parsers/coff/status.h>
#include <sindri/parsers/coff/utils.h>
#include <stdint.h>

snd_status_t snd_coff_get_symbol_name(const snd_coff_parser_t *parser, PSND_IMAGE_SYMBOL symbol, char *name_out,
                                      SIZE_T name_len) {
    SND_CHECK_NULL(parser, symbol, name_out, name_len);

    snd_memzero(name_out, name_len);

    if (symbol->N.Name.Short == 0) {
        DWORD offset = symbol->N.Name.Long;

        if (parser->string_table == NULL || offset < sizeof(DWORD) || offset >= parser->string_table_size) {
            return SND_ERR(SND_STATUS_SYMBOL_ENTRY_MISSING);
        }

        char  *long_name = (char *)SND_PTR_ADD(parser->string_table, offset);
        SIZE_T remaining = parser->string_table_size - offset;

        snd_strncpy(name_out, name_len, long_name, remaining);
    } else {
        SIZE_T short_len = snd_strnlen((const char *)symbol->N.ShortName, SND_IMAGE_SIZEOF_SHORT_NAME);
        snd_strncpy(name_out, name_len, (const char *)symbol->N.ShortName, short_len);
    }

    return SND_OK;
}

snd_status_t snd_coff_get_section_name(const snd_coff_parser_t *parser, PSND_IMAGE_SECTION_HEADER section,
                                       char *name_out, SIZE_T name_len) {
    SND_CHECK_NULL(parser, section, name_out, name_len);

    snd_memzero(name_out, name_len);

    if (section->Name[0] == '/') {
        uint32_t offset = 0;

        if (!snd_atou32_bounded((char *)&section->Name[1], SND_IMAGE_SIZEOF_SHORT_NAME - 1, &offset)) {
            return SND_ERR(SND_STATUS_SECTION_HEADER_MISSING);
        }

        if (parser->string_table == NULL || offset < sizeof(DWORD) || offset >= parser->string_table_size) {
            return SND_ERR(SND_STATUS_SECTION_HEADER_MISSING);
        }

        char  *long_name = (char *)SND_PTR_ADD(parser->string_table, offset);
        SIZE_T remaining = parser->string_table_size - offset;

        snd_strncpy(name_out, name_len, long_name, remaining);
    } else {
        SIZE_T short_len = snd_strnlen((const char *)section->Name, SND_IMAGE_SIZEOF_SHORT_NAME);
        snd_strncpy(name_out, name_len, (const char *)section->Name, short_len);
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

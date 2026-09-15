#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/common/string.h>
#include <sindri/internal/windows/pe.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/parsers/pe/section.h>

/**
 * @brief Parses an ASCII base-10 string table offset value from a COFF section
 * name.
 */
static DWORD parse_string_table_offset(const SND_IMAGE_SECTION_HEADER *section) {
    if (section->Name[0] != '/') {
        return 0;
    }

    const char *offset_str = (char *)(section->Name + 1);
    size_t      max_len    = SND_IMAGE_SIZEOF_SHORT_NAME - 1;

    uint32_t offset = 0;
    if (!snd_atou32_bounded(offset_str, max_len, &offset)) {
        return 0;
    }

    return (DWORD)offset;
}

void snd_pe_section_name(const snd_pe_parser_t *parser, const SND_IMAGE_SECTION_HEADER *section, char *name_buffer,
                         size_t buffer_size) {
    if (!name_buffer || buffer_size == 0) {
        return;
    }

    name_buffer[0] = '\0';

    if (!parser || !section) {
        return;
    }

    if (SND_PTR_BEFORE(section, parser->source.data) ||
        SND_RANGE_EXCEEDS(SND_PTR_DIFF(section, parser->source.data), sizeof(SND_IMAGE_SECTION_HEADER),
                          parser->source.size)) {
        snd_strncpy(name_buffer, buffer_size, SND_FALLBACK_STR("Corrupt Section"), SIZE_MAX);
        return;
    }

    if (section->Name[0] != '/') {
        size_t actual_len = 0;
        while (actual_len < SND_IMAGE_SIZEOF_SHORT_NAME && section->Name[actual_len] != '\0') {
            actual_len++;
        }

        size_t copy_len = SND_MIN(actual_len, buffer_size - 1);
        snd_memcpy(name_buffer, section->Name, copy_len);
        name_buffer[copy_len] = '\0';
        return;
    }

    if (parser->string_table == NULL || SND_PTR_BEFORE(parser->string_table, parser->source.data)) {
        snd_strncpy(name_buffer, buffer_size, SND_FALLBACK_STR("Unknown Section"), SIZE_MAX);
        return;
    }

    SIZE_T table_ptr_offset = SND_PTR_DIFF(parser->string_table, parser->source.data);
    if (table_ptr_offset >= parser->source.size) {
        snd_strncpy(name_buffer, buffer_size, SND_FALLBACK_STR("Corrupt Section"), SIZE_MAX);
        return;
    }

    SIZE_T max_available = parser->source.size - table_ptr_offset;
    if (max_available < sizeof(DWORD)) {
        snd_strncpy(name_buffer, buffer_size, SND_FALLBACK_STR("Corrupt Section"), SIZE_MAX);
        return;
    }

    DWORD claimed_table_size = 0;
    snd_memcpy(&claimed_table_size, parser->string_table, sizeof(DWORD));

    if (claimed_table_size > max_available) {
        claimed_table_size = (DWORD)max_available;
    }

    DWORD offset = parse_string_table_offset(section);

    if (offset < 4 || offset >= claimed_table_size) {
        snd_strncpy(name_buffer, buffer_size, SND_FALLBACK_STR("Corrupt Section"), SIZE_MAX);
        return;
    }

    const char *str_ptr      = (char *)SND_PTR_ADD(parser->string_table, offset);
    size_t      max_safe_len = claimed_table_size - offset;

    if (snd_strnlen(str_ptr, max_safe_len) == max_safe_len) {
        snd_strncpy(name_buffer, buffer_size, SND_FALLBACK_STR("Corrupt Section"), SIZE_MAX);
        return;
    }

    snd_strncpy(name_buffer, buffer_size, str_ptr, max_safe_len);
}

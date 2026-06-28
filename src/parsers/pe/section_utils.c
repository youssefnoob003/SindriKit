#include <sindri/common/debug.h>
#include <sindri/common/macros.h>
#include <sindri/common/memory.h>
#include <sindri/common/string.h>
#include <sindri/parsers/pe/section_utils.h>
#include <windows.h>

/**
 * @brief Parses an ASCII base-10 string table offset value from a COFF section
 * name. Replaces standard strtoul.
 */
static DWORD parse_string_table_offset(const IMAGE_SECTION_HEADER *section) {
    DWORD offset = 0;

    for (int i = 1; i < IMAGE_SIZEOF_SHORT_NAME; i++) {
        char c = (char)section->Name[i];
        if (c >= '0' && c <= '9') {
            if (offset > (0xFFFFFFFF / 10)) {
                return 0;
            }
            offset = (offset * 10) + (DWORD)(c - '0');
        } else if (c == '\0') {
            break;
        } else {
            return 0;
        }
    }
    return offset;
}

void snd_pe_section_name(const snd_pe_parser_t *parser, const IMAGE_SECTION_HEADER *section, char *name_buffer,
                         size_t buffer_size) {
    if (parser == NULL || section == NULL || name_buffer == NULL || buffer_size == 0) {
        if (name_buffer && buffer_size > 0) {
            name_buffer[0] = '\0';
        }
        return;
    }

    name_buffer[0] = '\0';

    if (section->Name[0] != '/') {
        size_t actual_len = 0;
        while (actual_len < IMAGE_SIZEOF_SHORT_NAME && section->Name[actual_len] != '\0') {
            actual_len++;
        }

        size_t copy_len = (buffer_size - 1 < actual_len) ? buffer_size - 1 : actual_len;
        snd_memcpy(name_buffer, section->Name, copy_len);
        name_buffer[copy_len] = '\0';
        return;
    }

    if (parser->string_table == NULL) {
        snd_strncpy(name_buffer, buffer_size, SND_FALLBACK_STR("Unknown Section"), 15);
        return;
    }

    SIZE_T table_ptr_offset = (SIZE_T)(parser->string_table - (BYTE *)parser->source.data);
    if (table_ptr_offset >= parser->source.size) {
        snd_strncpy(name_buffer, buffer_size, SND_FALLBACK_STR("Corrupt Section"), 15);
        return;
    }

    SIZE_T max_available = parser->source.size - table_ptr_offset;
    if (max_available < sizeof(DWORD)) {
        snd_strncpy(name_buffer, buffer_size, SND_FALLBACK_STR("Corrupt Section"), 15);
        return;
    }

    DWORD claimed_table_size = 0;
    snd_memcpy(&claimed_table_size, parser->string_table, sizeof(DWORD));

    if (claimed_table_size > max_available) {
        claimed_table_size = (DWORD)max_available;
    }

    unsigned long offset = parse_string_table_offset(section);

    if (offset < 4 || offset >= claimed_table_size) {
        snd_strncpy(name_buffer, buffer_size, SND_FALLBACK_STR("Corrupt Section"), 15);
        return;
    }

    const char *str_ptr      = (const char *)(parser->string_table + offset);
    size_t      max_safe_len = claimed_table_size - offset;

    if (snd_strnlen(str_ptr, max_safe_len) == max_safe_len) {
        snd_strncpy(name_buffer, buffer_size, SND_FALLBACK_STR("Corrupt Section"), 15);
        return;
    }

    snd_strncpy(name_buffer, buffer_size, str_ptr, max_safe_len);
}

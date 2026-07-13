#ifndef SND_PARSERS_COFF_PARSER_H
#define SND_PARSERS_COFF_PARSER_H

#include <sindri/common/buffer.h>
#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Context structure for the COFF parser.
 *
 * @note Pointers inside this structure point directly into the memory space
 * backed by the `source` buffer. They do not own memory and their lifecycles
 * are tied entirely to the validity of the underlying `source`.
 */
typedef struct {
    snd_buffer_t source;

    PIMAGE_FILE_HEADER    file_header;
    PIMAGE_SECTION_HEADER section_head;

    PIMAGE_SYMBOL symbol_table;
    DWORD         symbol_count;

    BYTE *string_table;
    DWORD string_table_size;

    BOOL  is_64bit;
    DWORD sections_count;
} snd_coff_parser_t;

/**
 * @brief Parses a raw COFF object file buffer into a parsed COFF representation.
 *
 * @param buf Raw buffer containing the COFF file data.
 * @param parser Pointer to the parser context to populate.
 * @return SND_OK on success, or a contextual error status.
 */
snd_status_t snd_coff_parse(const snd_buffer_t *buf, snd_coff_parser_t *parser);

SND_END_EXTERN_C

#endif // SND_PARSERS_COFF_PARSER_H

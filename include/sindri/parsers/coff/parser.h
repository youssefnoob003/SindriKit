#ifndef SND_PARSERS_COFF_PARSER_H
#define SND_PARSERS_COFF_PARSER_H

#include <sindri/common/buffer.h>
#include <sindri/common/macros.h>
#include <sindri/internal/windows/coff.h>
#include <sindri/status/core.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Context structure for the COFF parser.
 *
 * @note Pointers inside this structure point directly into the memory space
 * backed by the `source` buffer. They do not own memory and their lifecycles
 * are tied entirely to the validity of the underlying `source`.
 */
SND_SHUFFLE_START
typedef struct {
    snd_buffer_t source;

    PSND_IMAGE_FILE_HEADER    file_header;
    PSND_IMAGE_SECTION_HEADER section_head;

    PSND_IMAGE_SYMBOL symbol_table;
    DWORD             symbol_count;

    BYTE *string_table;
    DWORD string_table_size;

    BOOL  is_64bit;
    DWORD sections_count;
} snd_coff_parser_t;
SND_SHUFFLE_END

/**
 * @brief Parses a raw COFF object file buffer into a parsed COFF representation.
 *
 * @param buf Raw buffer containing the COFF file data.
 * @param parser Pointer to the parser context to populate.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p buf or @p parser is NULL.
 * @retval SND_STATUS_HEADER_FILE_TRUNCATED If the COFF header is truncated.
 * @retval SND_STATUS_HEADER_MACHINE_UNSUPPORTED If the machine type is not
 * supported.
 * @retval SND_STATUS_SECTION_TABLE_TRUNCATED If the section table is truncated.
 * @retval SND_STATUS_SYMBOL_TABLE_OVERFLOW If symbol-table arithmetic
 * overflows.
 * @retval SND_STATUS_SYMBOL_TABLE_TRUNCATED If the symbol table is truncated.
 */
snd_status_t snd_coff_parse(const snd_buffer_t *buf, snd_coff_parser_t *parser);

SND_END_EXTERN_C

#endif // SND_PARSERS_COFF_PARSER_H

#ifndef SND_PARSERS_COFF_UTILS_H
#define SND_PARSERS_COFF_UTILS_H

#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/coff/parser.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Retrieves the actual name of a COFF symbol.
 *
 * COFF symbols store short names (<= 8 chars) inline in the Name field.
 * Longer names are stored in the String Table, and the Name field contains
 * a union indicating the offset into that table. This function abstracts
 * that away and always returns a pointer to a null-terminated string, or
 * copies it into a provided buffer.
 *
 * @param parser The parsed COFF object.
 * @param symbol The symbol structure to evaluate.
 * @param name_out Output buffer to store the name.
 * @param name_len Size of the output buffer.
 * @return SND_OK on success, or an error status.
 */
snd_status_t snd_coff_get_symbol_name(const snd_coff_parser_t *parser, PIMAGE_SYMBOL symbol, char *name_out,
                                      SIZE_T name_len);

/**
 * @brief Retrieves the actual name of a COFF section.
 *
 * Like symbols, sections with long names (e.g. "/4") are indices into
 * the string table.
 *
 * @param parser The parsed COFF object.
 * @param section The section header.
 * @param name_out Output buffer to store the name.
 * @param name_len Size of the output buffer.
 * @return SND_OK on success.
 */
snd_status_t snd_coff_get_section_name(const snd_coff_parser_t *parser, PIMAGE_SECTION_HEADER section, char *name_out,
                                       SIZE_T name_len);

/**
 * @brief Abstracted address translation. Converts a raw file offset to a pointer.
 * @param parser The parsed COFF image.
 * @param raw_offset The raw file offset (PointerToRawData).
 * @param size The size of the expected data at the offset for bounds checking.
 * @return A direct pointer to the data, or NULL if out of bounds.
 */
PVOID snd_coff_raw_to_ptr(const snd_coff_parser_t *parser, DWORD raw_offset, SIZE_T size);

SND_END_EXTERN_C

#endif // SND_PARSERS_COFF_UTILS_H

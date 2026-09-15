#ifndef SND_PARSERS_COFF_RELOCATIONS_H
#define SND_PARSERS_COFF_RELOCATIONS_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/coff.h>
#include <sindri/parsers/coff/parser.h>
#include <sindri/status/core.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Retrieves the array of relocations for a specific section.
 *
 * @param parser The parsed COFF object.
 * @param section The section header to retrieve relocations for.
 * @param relocations_out Output pointer to the start of the IMAGE_RELOCATION array.
 * @param count_out Output pointer for the number of relocations in the array.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If an input or output pointer is NULL.
 * @retval SND_STATUS_RELOCATION_TABLE_MISSING If the section has no relocation
 * table.
 * @retval SND_STATUS_RELOCATION_TABLE_INVALID If the relocation table is
 * invalid.
 * @retval SND_STATUS_RELOCATION_TABLE_OVERFLOW If relocation bounds arithmetic
 * overflows.
 * @retval SND_STATUS_RELOCATION_TABLE_TRUNCATED If the relocation table is
 * truncated.
 */
snd_status_t snd_coff_get_relocations(const snd_coff_parser_t *parser, const SND_IMAGE_SECTION_HEADER *section,
                                      PSND_IMAGE_RELOCATION *relocations_out, DWORD *count_out);

/**
 * @brief Returns the patch width for a COFF relocation type.
 * @param is_64bit TRUE for AMD64 relocation types, FALSE for i386 types.
 * @param reloc_type Relocation type to inspect.
 * @param size_out Receives the patch width in bytes.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p size_out is NULL.
 * @retval SND_STATUS_COFF_RELOCATION_TYPE_UNSUPPORTED If the relocation type
 * is unsupported.
 */
snd_status_t snd_coff_get_relocation_patch_size(BOOL is_64bit, WORD reloc_type, SIZE_T *size_out);

SND_END_EXTERN_C

#endif // SND_PARSERS_COFF_RELOCATIONS_H

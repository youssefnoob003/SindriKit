#ifndef SND_PARSERS_PE_RELOCATIONS_H
#define SND_PARSERS_PE_RELOCATIONS_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/pe.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/status/core.h>

SND_BEGIN_EXTERN_C

#define SND_PAGE_OFFSET_MASK 0x0FFF

/**
 * @brief Represents a parsed base relocation entry.
 * Contains information required to apply a fixup patch to an in-memory PE image.
 */
typedef struct {
    WORD  type;      // Relocation type (e.g., SND_IMAGE_REL_BASED_HIGHLOW, SND_IMAGE_REL_BASED_DIR64)
    DWORD patch_rva; // RVA where the relocation fixup patch needs to be applied
    PVOID patch_ptr; // Pointer in host memory to the location being patched (NULL for ABSOLUTE)
} snd_pe_reloc_entry_t;

/**
 * @brief Iterates and retrieves the next base relocation block from the PE Base Relocation Directory.
 *
 * Traverses `SND_IMAGE_BASE_RELOCATION` blocks sequentially using a caller-managed cursor.
 * Performs strict validation on block headers and boundary ranges.
 *
 * @param parser Pointer to the initialized PE parser context.
 * @param cursor Pointer to a tracking offset state variable. Set to 0 on initial call;
 *               will be updated automatically to point to the next block on success.
 * @param out_block Pointer to receive the pointer to the parsed `SND_IMAGE_BASE_RELOCATION` header.
 *                  Set to NULL if no relocations exist or the end of the directory is reached.
 * @param out_entries_count Pointer to receive the total number of relocation entries in this block.
 *
 * @retval SND_OK On success, or when reaching the end of the relocation table / if relocations are absent.
 * @retval SND_STATUS_NULL_POINTER If `parser`, `cursor`, `out_block`, or `out_entries_count` is NULL.
 * @retval SND_STATUS_RELOCATION_DIRECTORY_TRUNCATED If relocation directory size is smaller than header size.
 * @retval SND_STATUS_RELOCATION_DIRECTORY_OVERFLOW If calculating directory RVA bounds triggers DWORD overflow.
 * @retval SND_STATUS_RELOCATION_DIRECTORY_OUT_OF_BOUNDS If block header or contents extend beyond image memory.
 * @retval SND_STATUS_RELOCATION_BLOCK_INVALID If `SizeOfBlock` is smaller than header size or exceeds bounds.
 */
snd_status_t snd_pe_get_reloc_block(const snd_pe_parser_t *parser, SIZE_T *cursor,
                                    const SND_IMAGE_BASE_RELOCATION **out_block, DWORD *out_entries_count);

/**
 * @brief Parses a specific relocation entry within a base relocation block.
 *
 * Decodes entry type and offset, calculates the final target patch RVA, and resolves
 * a host virtual address pointer to the memory location requiring the relocation fixup.
 *
 * @param parser Pointer to the initialized PE parser context.
 * @param block Pointer to the valid `SND_IMAGE_BASE_RELOCATION` block header.
 * @param entry_index The zero-based index of the entry within the specified block.
 * @param out_entry Pointer to the `snd_pe_reloc_entry_t` structure to populate with entry details.
 *
 * @retval SND_OK On successfully parsing entry details (including `SND_IMAGE_REL_BASED_ABSOLUTE` padding).
 * @retval SND_STATUS_NULL_POINTER If `parser`, `block`, or `out_entry` is NULL.
 * @retval SND_STATUS_RELOCATION_ENTRY_OVERFLOW If `entry_index` exceeds total block entries or RVA math overflows.
 * @retval SND_STATUS_PE_RELOCATION_TYPE_UNSUPPORTED If relocation type is unrecognised or unsupported.
 * @retval SND_STATUS_PE_RELOCATION_PATCH_OUT_OF_RANGE If target patch RVA resolves outside mapped image memory.
 */
snd_status_t snd_pe_get_reloc_entry(const snd_pe_parser_t *parser, const SND_IMAGE_BASE_RELOCATION *block,
                                    DWORD entry_index, snd_pe_reloc_entry_t *out_entry);

SND_END_EXTERN_C

#endif // SND_PARSERS_PE_RELOCATIONS_H

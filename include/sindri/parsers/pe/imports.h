#ifndef SND_PARSERS_PE_IMPORTS_H
#define SND_PARSERS_PE_IMPORTS_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/pe.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/primitives/os_api.h>
#include <sindri/status/core.h>

SND_BEGIN_EXTERN_C

/**
 * @name PE Ordinal Flag Masks
 * @brief Utilities for parsing `IMAGE_THUNK_DATA` entries within the Import
 * Name Table (INT). According to the Portable Executable specification, if the
 * highest-order bit (MSB) of a thunk entry is set, the function is imported by
 * its numerical ordinal rather than by a string name.
 */

// Checks if a 32-bit thunk entry matches an import-by-ordinal (MSB is set).
#define SND_PE_SNAP_BY_ORDINAL32(ordinal) ((ordinal) & 0x80000000)

// Checks if a 64-bit thunk entry matches an import-by-ordinal (MSB is set).
#define SND_PE_SNAP_BY_ORDINAL64(ordinal) ((ordinal) & 0x8000000000000000ULL)

// Strips the ordinal flag bit to extract the clean, 16-bit numerical ordinal
// ID.
#define SND_PE_ORDINAL(ordinal) ((ordinal) & 0xFFFF)

/**
 * @brief Represents a parsed and normalized import thunk entry.
 * Abstracts the differences between 32-bit and 64-bit thunks and whether
 * the target function is imported by name or by ordinal.
 */
typedef struct {
    const char *name;       // Target function name (NULL if imported by ordinal)
    WORD        ordinal;    // Ordinal number (0 if imported by name)
    BOOL        is_ordinal; // True if importing by ordinal
    PVOID       iat_slot;   // Pointer to the IAT entry in memory to write to
} snd_pe_import_thunk_t;

/**
 * @brief Retrieves a specific import descriptor from the PE's Import Directory.
 *
 * @param parser Pointer to the initialized PE parser context.
 * @param index The zero-based index of the import descriptor to retrieve.
 * @param out_desc Pointer to receive the `SND_IMAGE_IMPORT_DESCRIPTOR` pointer.
 *                 Will be set to NULL if the PE has no imports or if the index
 *                 points to the null-terminator descriptor indicating the end.
 *
 * @retval SND_OK On success (descriptor retrieved, or correctly reached the end/no imports).
 * @retval SND_STATUS_NULL_POINTER If @p parser or @p out_desc is NULL.
 * @retval SND_STATUS_IMPORT_DESCRIPTOR_OVERFLOW If computing the descriptor offset causes a math overflow.
 * @retval SND_STATUS_IMPORT_DESCRIPTOR_INVALID If the descriptor resides outside valid PE memory bounds.
 */
snd_status_t snd_pe_get_import_descriptor(const snd_pe_parser_t *parser, DWORD index,
                                          const SND_IMAGE_IMPORT_DESCRIPTOR **out_desc);

/**
 * @brief Resolves the target DLL name string from an import descriptor.
 *
 * @param parser Pointer to the initialized PE parser context.
 * @param desc Pointer to the valid import descriptor to read from.
 * @param out_name Pointer to receive the null-terminated ASCII string of the DLL name.
 *
 * @retval SND_OK On successfully resolving a safe, null-terminated DLL name.
 * @retval SND_STATUS_NULL_POINTER If @p parser, @p desc, or @p out_name is
 * NULL.
 * @retval SND_STATUS_IMPORT_NAME_INVALID If the descriptor Name RVA is 0, out of bounds,
 *                                        or the underlying string is missing a null-terminator.
 */
snd_status_t snd_pe_get_import_name(const snd_pe_parser_t *parser, const SND_IMAGE_IMPORT_DESCRIPTOR *desc,
                                    const char **out_name);

/**
 * @brief Retrieves and parses a specific import thunk from an import descriptor.
 *
 * Handles mapping between the OriginalFirstThunk (INT) and FirstThunk (IAT),
 * accounting for 32-bit vs 64-bit architecture differences, and bounds checks
 * the underlying function names if imported by name.
 *
 * @param parser Pointer to the initialized PE parser context.
 * @param desc Pointer to the valid import descriptor containing the thunks.
 * @param thunk_index The zero-based index of the thunk to retrieve.
 * @param out_thunk Pointer to the thunk structure to populate with parsed details.
 *                  If the end of the thunk array is reached, `iat_slot` and `name` will be NULL.
 *
 * @retval SND_OK On successful parse or gracefully hitting the null-terminator thunk.
 * @retval SND_STATUS_NULL_POINTER If @p parser, @p desc, or @p out_thunk is
 * NULL.
 * @retval SND_STATUS_IMPORT_THUNK_OVERFLOW If offset calculations overflow or RVA math is invalid.
 * @retval SND_STATUS_IMPORT_THUNK_OUT_OF_BOUNDS If the INT/IAT pointers reside outside valid PE bounds.
 * @retval SND_STATUS_IMPORT_THUNK_INVALID If the `SND_IMAGE_IMPORT_BY_NAME` structure is inaccessible.
 * @retval SND_STATUS_IMPORT_NAME_INVALID If the target function name string is missing a null-terminator.
 */
snd_status_t snd_pe_get_import_thunk(const snd_pe_parser_t *parser, const SND_IMAGE_IMPORT_DESCRIPTOR *desc,
                                     DWORD thunk_index, snd_pe_import_thunk_t *out_thunk);

SND_END_EXTERN_C

#endif // SND_PARSERS_PE_IMPORTS_H

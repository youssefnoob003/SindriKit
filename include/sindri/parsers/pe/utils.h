#ifndef SND_PARSERS_PE_UTILS_H
#define SND_PARSERS_PE_UTILS_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/internal/windows/pe.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri/status/core.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Minimum standard file alignment mandated by the official Microsoft PE
 * specification for standard executables.
 */
#define SND_PE_MIN_FILE_ALIGNMENT 512

/**
 * @brief Safely retrieves a PE Data Directory entry by index.
 *
 * Checks table boundary limits, verifies buffer ranges, and returns the requested
 * `SND_IMAGE_DATA_DIRECTORY` structure.
 *
 * @param parser Pointer to the initialized PE parser context.
 * @param index Directory index to retrieve (e.g., `SND_IMAGE_DIRECTORY_ENTRY_IMPORT`).
 * @param dir_out Pointer to receive the retrieved `SND_IMAGE_DATA_DIRECTORY` structure.
 *
 * @retval SND_OK On successfully retrieving the directory entry.
 * @retval SND_STATUS_NULL_POINTER If `parser` or `dir_out` is NULL.
 * @retval SND_STATUS_DIRECTORY_ENTRY_MISSING If `index` exceeds available entries or `VirtualAddress` is 0.
 * @retval SND_STATUS_DIRECTORY_ENTRY_INVALID If directory headers extend beyond image memory bounds.
 */
snd_status_t snd_pe_get_directory(const snd_pe_parser_t *parser, DWORD index, SND_IMAGE_DATA_DIRECTORY *dir_out);

/**
 * @brief Translates a Relative Virtual Address (RVA) to a host memory pointer.
 *
 * Handles both memory-mapped images and raw file images (mapping section RVAs
 * to File Offset Addresses). Performs strict bounds checks against image source limits.
 *
 * @param parser Pointer to the initialized PE parser context.
 * @param rva Relative Virtual Address to convert.
 * @param size Minimum size of contiguous data expected at the RVA for bounds checking.
 *
 * @retval Pointer to the data at the target RVA.
 * @retval NULL If the RVA range is invalid or outside the source image.
 */
PVOID snd_pe_rva_to_ptr(const snd_pe_parser_t *parser, DWORD rva, SIZE_T size);

/**
 * @brief Resolves the memory address of the PE image entry point.
 *
 * @param parser Pointer to the initialized PE parser context.
 *
 * @retval Pointer to the entry point code.
 * @retval NULL If the entry point is absent or invalid.
 */
PVOID snd_pe_get_entry_point(const snd_pe_parser_t *parser);

/**
 * @brief Resolves the host memory pointer to the Thread Local Storage (TLS) callbacks array.
 *
 * Parses the TLS directory (supporting both 32-bit and 64-bit PE formats), converts the
 * callback table Virtual Address (VA) to an RVA, and resolves the pointer.
 *
 * @param parser Pointer to the initialized PE parser context.
 *
 * @retval Pointer to the TLS callbacks pointer array.
 * @retval NULL If the TLS directory is absent or invalid.
 */
PVOID snd_pe_get_tls_callbacks(const snd_pe_parser_t *parser);

/**
 * @brief Calculates Win32 memory protection flags (`PAGE_*`) for a relative page offset.
 *
 * Evaluates PE section characteristics intersecting with the specified offset and aggregates
 * section flags to determine the appropriate native protection value (e.g., `SND_PAGE_EXECUTE_READ`).
 *
 * @param pe Pointer to the initialized PE parser context.
 * @param page_offset Relative offset into the PE image memory.
 *
 * @retval SND_PAGE_* The protection flags calculated for the page.
 */
DWORD snd_pe_get_page_protection_flags(const snd_pe_parser_t *pe, SIZE_T page_offset);

SND_END_EXTERN_C

#endif // SND_PARSERS_PE_UTILS_H

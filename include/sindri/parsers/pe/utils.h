#ifndef SND_PARSERS_PE_UTILS_H
#define SND_PARSERS_PE_UTILS_H

#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/pe/parser.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

#define SND_PE_MIN_FILE_ALIGNMENT 512

/**
 * @brief Validates payload architecture compatibility.
 * @param is_64bit TRUE if the payload is 64-bit, FALSE otherwise.
 * @return TRUE if the architectures match, FALSE otherwise.
 */
BOOL snd_pe_compatibility_check(BOOL is_64bit);

/**
 * @brief Retrieves the absolute memory address of the PE Entry Point.
 *
 * @param parser The parsed PE image.
 * @return Pointer to the entry point, or NULL if invalid/not found.
 */
void *snd_pe_get_entry_point(const snd_pe_parser_t *parser);

/**
 * @brief Retrieves the TLS callbacks from the PE image.
 *
 * @param base_address The base memory address of the PE image.
 * @param parser The parsed PE image.
 * @return A pointer to the TLS callbacks, or NULL if not found.
 */
PVOID snd_pe_get_tls_callbacks(PVOID base_address, const snd_pe_parser_t *parser);

/**
 * @brief Executes TLS callbacks.
 * @param virtual_base The base address where the image is mapped.
 * @param parser The parsed PE image.
 * @param reason The reason for calling the TLS callbacks (e.g.,
 * DLL_PROCESS_ATTACH).
 * @return A status indicating success or failure.
 */
snd_status_t snd_pe_execute_tls_callbacks(PVOID virtual_base, const snd_pe_parser_t *parser, DWORD reason);

/**
 * @brief Abstracted address translation. Converts an RVA to a mapped or file
 * pointer.
 * @param parser The parsed PE image.
 * @param rva The Relative Virtual Address to convert.
 * @param size The size of the expected data at the RVA for bounds checking.
 * @return A direct pointer to the data, or NULL if out of bounds/invalid.
 */
PVOID snd_pe_rva_to_ptr(const snd_pe_parser_t *parser, DWORD rva, SIZE_T size);

/**
 * @brief Safely retrieves a PE Data Directory.
 * @param parser The parsed PE image.
 * @param index The directory index (e.g. IMAGE_DIRECTORY_ENTRY_IMPORT).
 * @param dir_out Output pointer to store the retrieved directory.
 * @return SND_OK on success, SND_STATUS_DIRECTORY_NOT_FOUND or other
 * errors.
 */
snd_status_t snd_pe_get_directory(const snd_pe_parser_t *parser, DWORD index, IMAGE_DATA_DIRECTORY *dir_out);

SND_END_EXTERN_C

#endif // SND_PARSERS_PE_UTILS_H

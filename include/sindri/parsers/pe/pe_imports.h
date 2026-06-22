#ifndef SND_PARSERS_PE_IMPORTS_H
#define SND_PARSERS_PE_IMPORTS_H

#include <sindri/common/helpers.h>
#include <sindri/common/status.h>
#include <sindri/parsers/pe/pe_parser.h>
#include <sindri/primitives/os_api.h>
#include <windows.h>

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
 * @brief Resolves the imports of a PE image by loading the required modules and
 * resolving their function addresses.
 * @param base_address The base memory address where the PE is mapped.
 * @param mod_api Pointer to the module API functions to use for resolving
 * imports.
 * @param parser Pointer to the PE parser to use for resolving imports.
 * @return SND_OK on success, otherwise an error status.
 */
snd_status_t snd_pe_resolve_imports(PVOID base_address, const snd_module_api_t *mod_api, const snd_pe_parser_t *parser);

SND_END_EXTERN_C

#endif // SND_PARSERS_PE_IMPORTS_H

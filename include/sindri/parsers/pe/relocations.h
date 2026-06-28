#ifndef SND_PARSERS_PE_RELOCATIONS_H
#define SND_PARSERS_PE_RELOCATIONS_H

#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/pe/parser.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Applies base relocations to a virtually mapped PE image.
 *
 * This function iterates through the Base Relocation Table (if present) and
 * patches hardcoded absolute memory addresses to align with the actual runtime
 * base address where the image was mapped.
 *
 * @note The memory mapped at `base_address` must have at least `PAGE_READWRITE`
 * permissions before calling this function, as it physically overwrites memory
 * inside the `.text` and `.data` sections.
 *
 * @param base_address The actual runtime base memory address where the PE is
 * currently mapped.
 * @param delta_offset The mathematical difference between the actual mapped
 * base address and the payload's preferred base address
 * (`OptionalHeader.ImageBase`). If this is 0, the function exits early.
 * @param parser Pointer to the validated PE parser context.
 * @return SND_OK on success, or a contextual error if the payload cannot be
 * relocated (e.g., RELOCS_STRIPPED is set).
 */
snd_status_t snd_pe_apply_relocations(PVOID base_address, LONG_PTR delta_offset, const snd_pe_parser_t *parser);

SND_END_EXTERN_C

#endif // SND_PARSERS_PE_RELOCATIONS_H

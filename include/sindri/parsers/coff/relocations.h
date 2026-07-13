#ifndef SND_PARSERS_COFF_RELOCATIONS_H
#define SND_PARSERS_COFF_RELOCATIONS_H

#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/parsers/coff/parser.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Retrieves the array of relocations for a specific section.
 *
 * @param parser The parsed COFF object.
 * @param section The section header to retrieve relocations for.
 * @param relocations_out Output pointer to the start of the IMAGE_RELOCATION array.
 * @param count_out Output pointer for the number of relocations in the array.
 * @return SND_OK on success, SND_STATUS_NOT_FOUND if the section has no relocations, or other errors.
 */
snd_status_t snd_coff_get_relocations(const snd_coff_parser_t *parser, const IMAGE_SECTION_HEADER *section,
                                      PIMAGE_RELOCATION *relocations_out, DWORD *count_out);

/*
 * @brief Applies the relocations to the section map using the symbol map and execution base.
 *
 * @param parser The parsed COFF object.
 * @param section_map The section map to apply relocations to.
 * @param symbol_map The symbol map used for resolving relocations.
 * @param execution_base The base address of the executable.
 * @return SND_OK on success, or an error code on failure.
 */
snd_status_t snd_coff_apply_relocations(const snd_coff_parser_t *parser, LPVOID *section_map, LPVOID *symbol_map,
                                        LPVOID execution_base);

SND_END_EXTERN_C

#endif // SND_PARSERS_COFF_RELOCATIONS_H

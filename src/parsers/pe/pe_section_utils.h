#ifndef SND_INTERNAL_PE_SECTION_UTILS_H
#define SND_INTERNAL_PE_SECTION_UTILS_H

#include <sindri/parsers/pe/pe_parser.h>
#include <stddef.h>
#include <windows.h>

/* * Macro block to strip diagnostic error string literals out of the .rdata
 * section entirely when compiling for the production/SILENT tier.
 */
#if SND_DEBUG
#define SND_FALLBACK_STR(debug_str) debug_str
#else
#define SND_FALLBACK_STR(debug_str) ""
#endif

/**
 * @brief Retrieves the name of a PE section safely, resolving COFF string
 * tables if needed.
 * @note Internal engine use only.
 */
void snd_i_section_name(const snd_pe_parser_t *parser, const IMAGE_SECTION_HEADER *section, char *name_buffer,
                        size_t buffer_size);

/**
 * @brief Calculates the exact size to copy from raw file to virtual memory.
 * @note Internal engine use only.
 */
SND_FORCE_INLINE DWORD snd_i_section_copy_size(const IMAGE_SECTION_HEADER *s) {
    if (s->Misc.VirtualSize == 0) {
        return s->SizeOfRawData;
    }
    return s->SizeOfRawData < s->Misc.VirtualSize ? s->SizeOfRawData : s->Misc.VirtualSize;
}

/**
 * @brief Calculates the final allocated size of a section in virtual memory.
 * @note Internal engine use only.
 */
SND_FORCE_INLINE DWORD snd_i_section_loaded_size(const IMAGE_SECTION_HEADER *s) {
    return s->Misc.VirtualSize != 0 ? s->Misc.VirtualSize : s->SizeOfRawData;
}

#endif // SND_INTERNAL_PE_SECTION_UTILS_H

#ifndef SND_PE_SECTION_UTILS_H
#define SND_PE_SECTION_UTILS_H

#include <sindri/parsers/pe/parser.h>
#include <stddef.h>
#include <windows.h>

/**
 * @brief Retrieves the name of a PE section safely, resolving COFF string
 * tables if needed.
 * @note Internal engine use only.
 */
void snd_pe_section_name(const snd_pe_parser_t *parser, const IMAGE_SECTION_HEADER *section, char *name_buffer,
                         size_t buffer_size);

/**
 * @brief Calculates the exact size to copy from raw file to virtual memory.
 * @note Internal engine use only.
 */
SND_FORCE_INLINE DWORD snd_pe_section_copy_size(const IMAGE_SECTION_HEADER *s) {
    if (s->Misc.VirtualSize == 0) {
        return s->SizeOfRawData;
    }
    return s->SizeOfRawData < s->Misc.VirtualSize ? s->SizeOfRawData : s->Misc.VirtualSize;
}

/**
 * @brief Calculates the final allocated size of a section in virtual memory.
 * @note Internal engine use only.
 */
SND_FORCE_INLINE DWORD snd_pe_section_loaded_size(const IMAGE_SECTION_HEADER *s) {
    return s->Misc.VirtualSize != 0 ? s->Misc.VirtualSize : s->SizeOfRawData;
}

#endif // SND_PE_SECTION_UTILS_H

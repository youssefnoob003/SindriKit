#ifndef SND_PE_SECTION_UTILS_H
#define SND_PE_SECTION_UTILS_H

#include <sindri/internal/windows/pe.h>
#include <sindri/parsers/pe/parser.h>
#include <stddef.h>

/**
 * @brief Maximum length of a PE section name, including null terminator.
 */
#define SND_PE_MAX_SECTION_NAME_LEN 256

/**
 * @brief Retrieves the name of a PE section safely, resolving COFF string
 * tables if needed.
 * @note Internal engine use only.
 */
void snd_pe_section_name(const snd_pe_parser_t *parser, const SND_IMAGE_SECTION_HEADER *section, char *name_buffer,
                         size_t buffer_size);

/**
 * @brief Calculates the exact size to copy from raw file to virtual memory.
 * @note Internal engine use only.
 */
SND_FORCE_INLINE DWORD snd_pe_section_copy_size(const SND_IMAGE_SECTION_HEADER *s) {
    if (!s) {
        return 0;
    }
    if (s->Misc.VirtualSize == 0) {
        return s->SizeOfRawData;
    }
    return (s->SizeOfRawData < s->Misc.VirtualSize) ? s->SizeOfRawData : s->Misc.VirtualSize;
}

/**
 * @brief Calculates the final allocated size of a section in virtual memory.
 * @note Internal engine use only.
 */
SND_FORCE_INLINE DWORD snd_pe_section_loaded_size(const SND_IMAGE_SECTION_HEADER *s) {
    if (!s) {
        return 0;
    }
    return (s->Misc.VirtualSize != 0) ? s->Misc.VirtualSize : s->SizeOfRawData;
}

#endif // SND_PE_SECTION_UTILS_H

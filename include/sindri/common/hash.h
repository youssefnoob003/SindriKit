#ifndef SND_COMMON_HASH_H
#define SND_COMMON_HASH_H

#include <sindri/common/macros.h>
#include <stdint.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Computes the configured hash of a standard ASCII string
 * (Case-Sensitive).
 * @note Use this for API names from the Export Directory.
 */
uint32_t snd_hash(const char *str);

/**
 * @brief Computes the configured hash of a standard ASCII string, converting to
 * lowercase.
 * @note Use this for DLL names found in the PE Import Directory.
 */
uint32_t snd_hash_lower(const char *str);

/**
 * @brief Computes the configured hash of a wide string, converting to lowercase
 * (Case-Insensitive).
 * @note Use this for finding DLL module names in the PEB.
 */
uint32_t snd_hash_wide_lower(const wchar_t *str);

SND_END_EXTERN_C

#endif // SND_COMMON_HASH_H

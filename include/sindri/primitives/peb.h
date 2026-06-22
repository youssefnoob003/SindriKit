#ifndef SND_PEB_H
#define SND_PEB_H

#include "sindri_hashes.h"

#include <sindri/common/helpers.h>
#include <sindri/common/status.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Safely locates the base address of a loaded module by walking the PEB.
 * @param module_name Case-insensitive target name (e.g., L"ntdll.dll")
 * @return Base pointer to the module, or NULL if not found.
 */
snd_status_t WINAPI snd_peb_get_module_base(const wchar_t *module_name, PVOID *out_base);

/**
 * @brief Safely locates the base address of a loaded module by walking the PEB
 * and comparing hashes.
 * @param module_hash The hash of the target name.
 * @return Base pointer to the module, or NULL if not found.
 */
snd_status_t WINAPI snd_peb_get_module_base_by_hash(DWORD module_hash, PVOID *out_base);

SND_END_EXTERN_C

#endif // SND_PEB_H

#ifndef SND_PARSERS_ENV_PEB_H
#define SND_PARSERS_ENV_PEB_H

#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/internal/nt/peb.h>
#include <sindri_hashes.h>
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
snd_status_t WINAPI snd_peb_get_module_base_hash(DWORD module_hash, PVOID *out_base);

/**
 * @brief Dynamically retrieves the current local process Environment Block (PEB).
 */
SND_FORCE_INLINE PSND_PEB snd_peb_get_local(void) {
#if defined(_M_X64) || defined(__x86_64__)
    // 64-bit Intel/AMD Windows
    return (PSND_PEB)__readgsqword(0x60);

#elif defined(_M_IX86) || defined(__i386__)
    // 32-bit Intel/AMD Windows
    return (PSND_PEB)__readfsdword(0x30);

#elif defined(_M_ARM64) || defined(__aarch64__)
    // 64-bit ARM Windows
    return (PSND_PEB)__readx18qword(0x60);

#else
#error "Unsupported CPU architecture for local PEB resolution."
#endif
}

SND_END_EXTERN_C

#endif // SND_PARSERS_ENV_PEB_H

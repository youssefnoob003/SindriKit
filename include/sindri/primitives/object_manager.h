#ifndef SND_PRIMITIVES_OBJECT_MANAGER_H
#define SND_PRIMITIVES_OBJECT_MANAGER_H

#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/primitives/os_api.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

#if defined(_WIN64)
#define SND_TARGET_KNOWNDLLS_DIR L"\\KnownDlls\\"
#elif defined(_WIN32)
#define SND_TARGET_KNOWNDLLS_DIR L"\\KnownDlls32\\"
#else
#error "Unsupported architecture: SindriKit requires _WIN32 or _WIN64"
#endif

/**
 * @brief Maps a module from the \KnownDlls object directory into memory.
 *
 * @param config Pointer to the injected API configuration (Win32 or Native).
 * @param dll_name The exact name of the DLL in the KnownDlls directory (e.g.,
 * L"ntdll.dll").
 * @param out_base_address Pointer to receive the base address of the mapped
 * section.
 * @return SND_OK on success, or an error code on failure.
 */
snd_status_t snd_om_knowndll_map(const snd_mapping_api_t *config, const wchar_t *dll_name, PVOID *out_base_address);

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_OBJECT_MANAGER_H

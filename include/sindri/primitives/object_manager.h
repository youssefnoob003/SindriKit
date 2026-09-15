#ifndef SND_PRIMITIVES_OBJECT_MANAGER_H
#define SND_PRIMITIVES_OBJECT_MANAGER_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/types.h>
#include <sindri/primitives/os_api.h>
#include <sindri/status.h>

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
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p config, @p dll_name, or
 * @p out_base_address is NULL.
 * @retval SND_STATUS_OM_NOT_INITIALIZED If the mapping API does not provide
 * both `open` and `view` callbacks.
 * @retval Any error returned by `snd_mapping_api_t::open` or
 * `snd_mapping_api_t::view`.
 */
snd_status_t snd_om_knowndll_map(const snd_mapping_api_t *config, const wchar_t *dll_name, PVOID *out_base_address);

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_OBJECT_MANAGER_H

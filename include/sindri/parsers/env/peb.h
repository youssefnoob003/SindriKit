#ifndef SND_PARSERS_ENV_PEB_H
#define SND_PARSERS_ENV_PEB_H

#include <sindri/common/macros.h>
#include <sindri/internal/nt/peb.h>
#include <sindri/internal/windows/types.h>
#include <sindri/parsers/env/status.h>
#include <sindri_hashes.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

SND_BEGIN_EXTERN_C

#define MAX_ITERATIONS 500
/**
 * @brief Safely locates the base address of a loaded module by walking the PEB.
 * @param module_name Case-insensitive target name (e.g., L"ntdll.dll")
 * @param out_base Receives the module base address.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p module_name or @p out_base is
 * NULL.
 * @retval Any error returned by the PEB module-list walker.
 */
snd_status_t WINAPI snd_peb_get_module_base(const wchar_t *module_name, PVOID *out_base);

/**
 * @brief Safely locates the base address of a loaded module by walking the PEB
 * and comparing hashes.
 * @param module_hash The hash of the target name.
 * @param out_base Receives the module base address.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p out_base is NULL or
 * @p module_hash is 0.
 * @retval Any error returned by the PEB module-list walker.
 */
snd_status_t WINAPI snd_peb_get_module_base_hash(DWORD module_hash, PVOID *out_base);

/**
 * @brief Retrieves the current process parameters from a PEB.
 *
 * @param peb PEB to inspect, or NULL to use the local PEB.
 * @param out_params Receives the process-parameters structure.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p out_params, the PEB, or the process
 * parameters are absent.
 * @retval SND_STATUS_PEB_GET_FAILED If the local PEB cannot be retrieved.
 * @retval SND_STATUS_PROCESS_PARAMS_NOT_FOUND If process parameters are not
 * initialized.
 */
snd_status_t WINAPI snd_env_get_process_params(const PSND_PEB peb, PSND_RTL_USER_PROCESS_PARAMETERS *out_params);

/**
 * @brief Retrieves the command-line Unicode string from a PEB.
 *
 * @param peb PEB to inspect, or NULL to use the local PEB.
 * @param out_cmd_line Receives the command-line string descriptor.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p out_cmd_line, the PEB, process
 * parameters, or command line are absent.
 * @retval Any error returned by `snd_env_get_process_params`.
 */
snd_status_t WINAPI snd_env_get_command_line(const PSND_PEB peb, SND_UNICODE_STRING **out_cmd_line);

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

/**
 * @brief Dynamically retrieves the WOW32Reserved pointer from the local TEB (fs:[0xC0]).
 */
SND_FORCE_INLINE PVOID snd_env_get_wow32_reserved(void) {
#if defined(_M_IX86) || defined(__i386__)
    return (PVOID)(ULONG_PTR)__readfsdword(0xC0);
#else
    return NULL;
#endif
}

SND_END_EXTERN_C

#endif // SND_PARSERS_ENV_PEB_H

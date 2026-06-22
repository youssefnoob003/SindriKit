#ifndef SND_LOADERS_KNOWNDLLS_H
#define SND_LOADERS_KNOWNDLLS_H

#include <sindri/common/helpers.h>
#include <sindri/common/status.h>
#include <sindri/internal/nt_defs.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

#if defined(_WIN64)
#define SND_TARGET_KNOWNDLLS_DIR L"\\KnownDlls\\"
#elif defined(_WIN32)
#define SND_TARGET_KNOWNDLLS_DIR L"\\KnownDlls32\\"
#else
#error "Unsupported architecture: SindriKit requires _WIN32 or _WIN64"
#endif

typedef snd_status_t(WINAPI *snd_knowndlls_open_section_cb)(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess,
                                                            PSND_OBJECT_ATTRIBUTES ObjectAttributes);

typedef snd_status_t(WINAPI *snd_knowndlls_map_view_cb)(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress,
                                                        ULONG_PTR ZeroBits, SIZE_T CommitSize,
                                                        PLARGE_INTEGER SectionOffset, PSIZE_T ViewSize,
                                                        DWORD InheritDisposition, ULONG AllocationType,
                                                        ULONG Win32Protect);

typedef snd_status_t(WINAPI *snd_knowndlls_close_cb)(HANDLE Handle);

/**
 * @brief Configuration structure for the KnownDlls loader mapping technique.
 */
typedef struct {
    snd_knowndlls_open_section_cb open_section;
    snd_knowndlls_map_view_cb     map_view_of_section;
    snd_knowndlls_close_cb        close_handle;
} snd_knowndlls_config_t;

// Expose the available configurations to the caller
extern const snd_knowndlls_config_t snd_knowndlls_win;
extern const snd_knowndlls_config_t snd_knowndlls_native;

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
snd_status_t snd_map_knowndll(const snd_knowndlls_config_t *config, const wchar_t *dll_name, PVOID *out_base_address);

SND_END_EXTERN_C

#endif // SND_LOADERS_KNOWNDLLS_H

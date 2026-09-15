#ifndef SND_PRIMITIVES_FILES_H
#define SND_PRIMITIVES_FILES_H

#include <sindri/common/buffer.h>
#include <sindri/common/macros.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Callback to load a file into a buffer.
 *
 * @param path The path to the file to load.
 * @param out_buffer Pointer to a buffer structure that will receive the loaded file data.
 * @return `SND_SUCCESS` on success, or a relevant error code.
 */
typedef snd_status_t(WINAPI *snd_file_load_cb)(const char *path, snd_buffer_t *out_buffer);

/**
 * @brief File primitive operations API.
 */
typedef struct {
    snd_file_load_cb load;
} snd_file_api_t;

/**
 * @brief Win32 file primitive backend.
 * Uses standard `CreateFileA` / `ReadFile`. Supports relative and absolute paths.
 */
extern const snd_file_api_t snd_file_win;

/**
 * @brief Native API file primitive backend.
 * Uses `NtCreateFile` / `NtReadFile`. 
 *
 * @note **Important:** This backend requires absolute paths (e.g., `C:\path\to\file.ext`).
 * Relative paths are not supported because the native path prefix (`\??\`) requires a fully qualified absolute DOS path to resolve correctly without a root directory handle.
 */
extern const snd_file_api_t snd_file_nt;

/**
 * @brief Direct syscall file primitive backend.
 * Bypasses `ntdll.dll` using direct system calls for `NtCreateFile` / `NtReadFile`.
 *
 * @note **Important:** Like the NT backend, this backend requires absolute paths (e.g., `C:\path\to\file.ext`).
 * Relative paths are not supported.
 */
extern const snd_file_api_t snd_file_sys;

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_FILES_H

#ifndef SND_PRIMITIVES_OS_API_H
#define SND_PRIMITIVES_OS_API_H

#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

// Memory Capabilities
typedef snd_status_t(WINAPI *snd_memory_alloc_cb)(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protect,
                                                  LPVOID *out_address);
typedef snd_status_t(WINAPI *snd_memory_free_cb)(LPVOID address, SIZE_T size, DWORD free_type);
typedef snd_status_t(WINAPI *snd_memory_protect_cb)(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect);

// Module Capabilities
typedef snd_status_t(WINAPI *snd_module_load_cb)(const char *module_name, HMODULE *out_module);
typedef snd_status_t(WINAPI *snd_module_get_proc_cb)(HMODULE hModule, const char *proc_name, FARPROC *out_proc);
typedef snd_status_t(WINAPI *snd_module_resolver_cb)(const wchar_t *module_name, PVOID *out_base);

// Module Capabilities (Hashes)
typedef snd_status_t(WINAPI *snd_module_get_proc_hash_cb)(HMODULE hModule, DWORD proc_hash, FARPROC *out_proc);
typedef snd_status_t(WINAPI *snd_module_resolver_hash_cb)(DWORD module_hash, PVOID *out_base);

// Mapping Capabilities
typedef snd_status_t(WINAPI *snd_mapping_open_cb)(const wchar_t *section_name, HANDLE *out_handle);
typedef snd_status_t(WINAPI *snd_mapping_view_cb)(HANDLE section_handle, PVOID *out_base, SIZE_T *out_size);
typedef snd_status_t(WINAPI *snd_mapping_close_cb)(HANDLE handle);

// Process Capabilities
typedef snd_status_t(WINAPI *snd_process_open_cb)(DWORD pid, DWORD desired_access, HANDLE *out_process);
typedef snd_status_t(WINAPI *snd_process_alloc_remote_cb)(HANDLE process, SIZE_T size, DWORD allocation_type,
                                                          DWORD protect, PVOID *out_address);
typedef snd_status_t(WINAPI *snd_process_write_remote_cb)(HANDLE process, PVOID base_address, const void *buffer,
                                                          SIZE_T size, SIZE_T *bytes_written);
typedef snd_status_t(WINAPI *snd_process_protect_remote_cb)(HANDLE process, PVOID base_address, SIZE_T size,
                                                            DWORD new_protect, DWORD *old_protect);
typedef snd_status_t(WINAPI *snd_process_create_thread_cb)(HANDLE process, PVOID start_address, PVOID parameter,
                                                           HANDLE *out_thread);
typedef snd_status_t(WINAPI *snd_process_close_cb)(HANDLE handle);

/**
 * @brief Local Memory Management API table.
 */
SND_SHUFFLE_START
typedef struct {
    snd_memory_alloc_cb   alloc;
    snd_memory_free_cb    free;
    snd_memory_protect_cb protect;
} snd_memory_api_t;
SND_SHUFFLE_END

/**
 * @brief Module and Import Resolution API table.
 */
SND_SHUFFLE_START
typedef struct {
    // String-based resolution
    snd_module_load_cb     load_library;
    snd_module_get_proc_cb get_proc_address;
    snd_module_resolver_cb get_module_base;

    // Hash-based resolution
    snd_module_get_proc_hash_cb get_proc_address_hash;
    snd_module_resolver_hash_cb get_module_base_hash;
} snd_module_api_t;
SND_SHUFFLE_END

/**
 * @brief Mapping API table.
 */
SND_SHUFFLE_START
typedef struct {
    snd_mapping_open_cb  open;
    snd_mapping_view_cb  view;
    snd_mapping_close_cb close;
} snd_mapping_api_t;
SND_SHUFFLE_END

/**
 * @brief Remote Process Operations API table.
 */
SND_SHUFFLE_START
typedef struct {
    snd_process_open_cb           open_process;
    snd_process_alloc_remote_cb   alloc_remote;
    snd_process_write_remote_cb   write_remote;
    snd_process_protect_remote_cb protect_remote;
    snd_process_create_thread_cb  create_remote_thread;
    snd_process_close_cb          close_handle;
} snd_process_api_t;
SND_SHUFFLE_END

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_OS_API_H

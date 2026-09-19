#ifndef SND_PRIMITIVES_OS_API_H
#define SND_PRIMITIVES_OS_API_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/constants.h>
#include <sindri/internal/windows/types.h>
#include <sindri/status.h>

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
typedef struct snd_process_api snd_process_api_t; // Forward declaration

typedef snd_status_t(WINAPI *snd_process_create_params_cb)(const void *nt_path_unicode, const wchar_t *cmd_line,
                                                           PVOID *out_params);
typedef snd_status_t(WINAPI *snd_process_free_params_cb)(PVOID params);

typedef snd_status_t(WINAPI *snd_process_create_cb)(const snd_process_api_t *api, const wchar_t *image_path,
                                                    const wchar_t *command_line, HANDLE *out_process,
                                                    HANDLE *out_thread);
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

// Thread Capabilities
typedef struct snd_thread_registers SND_THREAD_REGISTERS; // Forward declaration

typedef snd_status_t(WINAPI *snd_thread_queue_apc_cb)(HANDLE thread, PVOID apc_routine, PVOID apc_argument);
typedef snd_status_t(WINAPI *snd_thread_resume_cb)(HANDLE thread);
typedef snd_thread_resume_cb snd_thread_suspend_cb;
typedef snd_status_t(WINAPI *snd_thread_get_context_cb)(HANDLE thread, SND_THREAD_REGISTERS *out_regs);
typedef snd_status_t(WINAPI *snd_thread_set_context_cb)(HANDLE thread, const SND_THREAD_REGISTERS *in_regs);
typedef snd_process_close_cb snd_thread_close_cb;

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
struct snd_process_api {
    snd_process_create_params_cb  create_process_params;
    snd_process_free_params_cb    free_process_params;
    snd_process_create_cb         create_process;
    snd_process_open_cb           open_process;
    snd_process_alloc_remote_cb   alloc_remote;
    snd_process_write_remote_cb   write_remote;
    snd_process_protect_remote_cb protect_remote;
    snd_process_create_thread_cb  create_remote_thread;
    snd_process_close_cb          close_handle;
};
SND_SHUFFLE_END

/**
 * @brief Thread Operations API table.
 */
SND_SHUFFLE_START
typedef struct {
    snd_thread_queue_apc_cb   queue_apc;
    snd_thread_resume_cb      resume_thread;
    snd_thread_suspend_cb     suspend_thread;
    snd_thread_get_context_cb get_context;
    snd_thread_set_context_cb set_context;
    snd_thread_close_cb       close_handle;
} snd_thread_api_t;
SND_SHUFFLE_END

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_OS_API_H

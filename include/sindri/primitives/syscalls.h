#ifndef SND_PRIMITIVES_SYSCALLS_COMMON_H
#define SND_PRIMITIVES_SYSCALLS_COMMON_H

#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <sindri/primitives/os_api.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

#define SND_MAX_SYSCALLS     500
#define SND_MAX_SYS_NAME_LEN 256

/**
 * @brief Syscall entry structure holding the address, hash, and syscall number.
 */
typedef struct {
    PVOID pAddress;
    DWORD dwHash;
    WORD  wSystemCall;
} snd_syscall_entry_t;

/**
 * @brief Arguments struct passed to the generic ASM syscall invoker.
 */
typedef struct {
    WORD  ssn;
    PVOID arg1;
    PVOID arg2;
    PVOID arg3;
    PVOID arg4;
    PVOID arg5;
    PVOID arg6;
    PVOID arg7;
    PVOID arg8;
    PVOID arg9;
    PVOID arg10;
    PVOID arg11;
} snd_syscall_args_t;

/**
 * @brief Universal function signature for any syscall resolution strategy.
 * Any custom or future gate can be plugged into the engine if it matches this.
 */
typedef snd_status_t (*snd_syscall_resolver_t)(PVOID ntdll_base, DWORD func_hash, snd_syscall_entry_t *entry_out);

snd_status_t snd_syscall_resolve_ssn_scan(PVOID ntdll, DWORD func_hash, snd_syscall_entry_t *entry_out);
snd_status_t snd_syscall_resolve_ssn_sort(PVOID ntdll, DWORD func_hash, snd_syscall_entry_t *entry_out);

/**
 * @brief Sets the primary syscall resolution strategy using a function pointer.
 * @param resolver The resolver function to use as the primary strategy.
 */
void snd_syscall_strategy_set(snd_syscall_resolver_t resolver);

/**
 * @brief Adds a fallback syscall resolution strategy to the pipeline.
 * @param resolver The fallback resolver function to add.
 * @return SND_OK on success, or an error if the pipeline is full.
 */
snd_status_t snd_syscall_strategy_add(snd_syscall_resolver_t resolver);

/**
 * @brief Sets the global base address of the ntdll.dll module.
 * @param ntdll_base The base address of the ntdll module.
 */
void snd_syscall_set_ntdll(PVOID ntdll_base);

/**
 * @brief Core resolution entrypoint (evaluates the internal fallback chain
 * automatically).
 *
 * @param func_hash The hash of the target syscall function name.
 * @param entry_out Pointer to the entry structure to populate.
 * @return SND_OK on success, or an error code if resolution fails.
 */
snd_status_t snd_syscall_resolve(DWORD func_hash, snd_syscall_entry_t *entry_out);

/**
 * @brief ASM stub for invoking syscalls.
 * @param args Pointer to the syscall arguments structure.
 * @return The NTSTATUS returned by the syscall.
 */
extern NTSTATUS snd_syscall_invoke_asm(snd_syscall_args_t *args);

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_SYSCALLS_COMMON_H

#ifndef SND_PRIMITIVES_SYSCALLS_COMMON_H
#define SND_PRIMITIVES_SYSCALLS_COMMON_H

#include <sindri/common/macros.h>
#include <sindri/internal/windows/types.h>
#include <sindri/primitives/os_api.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

#define SND_MAX_SYSCALLS            500
#define SND_MAX_SYS_NAME_LEN        256
#define SND_MAX_INTERNAL_STRATEGIES 4

// Call stack spoofing bounds
// Note: These limits are tightly coupled to the static stack allocation inside the ASM stub
// (e.g., `sub rsp, 512` in spoofed_x64.asm). Do not increase these limits without also
// proportionally increasing the .ALLOCSTACK value in the assembly files to prevent stack corruption.
#define SND_SPOOF_MAX_GADGET_SCAN   65535
#define SND_SPOOF_FRAME_MIN_SIZE    120
#define SND_SPOOF_FRAME_MAX_SIZE    496
#define SND_SPOOF_MAX_FUNCTION_SIZE 0x10000

typedef enum _SND_SYSCALL_STATUS_CODE {
    SND_STATUS_SSN_NOT_FOUND = SND_MAKE_STATUS(SND_FACILITY_SYSCALL, 1),
    SND_STATUS_GADGET_NOT_FOUND,
    SND_STATUS_SYSCALL_PIPELINE_EXHAUSTED,
    SND_STATUS_RESOLVER_NOT_INITIALIZED,
    SND_STATUS_NTDLL_NOT_INITIALIZED,
    SND_STATUS_SPOOF_GADGET_NOT_FOUND,
} snd_syscall_status_code_t;

const char *snd_syscall_status_to_string(int code);

#ifndef SND_USE_DEFAULTS
#define SND_USE_DEFAULTS 0
#endif

/**
 * @brief Syscall entry structure holding the address, hash, and syscall number.
 */
SND_SHUFFLE_START
typedef struct {
    PVOID pAddress;
    DWORD dwHash;
    WORD  wSystemCall;
    PVOID pSyscallAddr;
    PVOID pSpoofAddr;
    DWORD dwSpoofFrameSize; // The actual frame size of the Fat Frame in bytes
} snd_syscall_entry_t;
SND_SHUFFLE_END

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
    // Placed at the end to prevent altering the memory offsets of args 1-11
    // inside the existing Direct ASM stub (invoke_x64.asm).
    PVOID sys_addr;
    PVOID spoof_addr;
    DWORD spoof_frame_size;
} snd_syscall_args_t;

/**
 * @brief Universal function signature for any syscall resolution strategy.
 * Any custom or future gate can be plugged into the engine if it matches this.
 */
typedef snd_status_t (*snd_syscall_resolver_t)(DWORD func_hash, snd_syscall_entry_t *entry_out);
typedef NTSTATUS (*snd_syscall_invoker_t)(snd_syscall_args_t *args);
typedef snd_status_t (*snd_syscall_gadget_finder_t)(snd_syscall_entry_t *entry);

/**
 * @brief Resolves a syscall number by scanning the active NTDLL export stub.
 * @param func_hash Hash of the target syscall name.
 * @param entry_out Receives the resolved syscall entry.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p entry_out is NULL.
 * @retval SND_STATUS_SSN_NOT_FOUND If the syscall cannot be resolved.
 * @retval Any error returned by `snd_ntdll_get_clean_base`,
 * `snd_ntdll_get_clean_parser`, or `snd_ntdll_get_clean_export`.
 */
snd_status_t snd_syscall_resolve_ssn_scan(DWORD func_hash, snd_syscall_entry_t *entry_out);

/**
 * @brief Resolves a syscall number by sorting active NTDLL export stubs.
 * @param func_hash Hash of the target syscall name.
 * @param entry_out Receives the resolved syscall entry.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p entry_out is NULL.
 * @retval SND_STATUS_SSN_NOT_FOUND If the syscall cannot be resolved.
 * @retval Any error returned by `snd_ntdll_get_clean_parser` or
 * `snd_pe_enumerate_exports` while building the sorted table.
 */
snd_status_t snd_syscall_resolve_ssn_sort(DWORD func_hash, snd_syscall_entry_t *entry_out);

/**
 * @brief Finds a syscall transition gadget for an entry.
 * @param entry Syscall entry to update.
 * @retval SND_OK If a suitable gadget is found.
 * @retval SND_STATUS_NULL_POINTER If @p entry is NULL.
 * @retval SND_STATUS_GADGET_NOT_FOUND If no suitable gadget is found.
 * @retval Any error returned by `snd_ntdll_get_active_export`.
 */
snd_status_t snd_syscall_find_gadget_scan(snd_syscall_entry_t *entry);

/**
 * @brief Finds a spoofing gadget and frame metadata for an entry.
 * @param entry Syscall entry to update.
 * @retval SND_OK If spoofing metadata is found.
 * @retval SND_STATUS_NULL_POINTER If @p entry is NULL.
 * @retval SND_STATUS_SPOOF_GADGET_NOT_FOUND If no spoofing gadget is found.
 * @retval Any error returned by `snd_peb_get_module_base_hash`,
 * `snd_pe_parse`, or `snd_pe_get_export_address_hash`.
 */
snd_status_t snd_syscall_find_spoof_scan(snd_syscall_entry_t *entry);

#if SND_USE_DEFAULTS
#define SND_SYSCALL_INVOKER_DEFAULT       snd_syscall_indirect_invoke_asm
#define SND_SYSCALL_GADGET_FINDER_DEFAULT snd_syscall_find_gadget_scan
#define SND_SYSCALL_SPOOF_FINDER_DEFAULT  snd_syscall_find_spoof_scan
#define SND_SYSCALL_RESOLVER_DEFAULT      snd_syscall_resolve_ssn_scan
#else
#define SND_SYSCALL_INVOKER_DEFAULT       NULL
#define SND_SYSCALL_GADGET_FINDER_DEFAULT NULL
#define SND_SYSCALL_SPOOF_FINDER_DEFAULT  NULL
#define SND_SYSCALL_RESOLVER_DEFAULT      NULL
#endif

/**
 * @brief Sets the primary syscall resolution strategy using a function pointer.
 * @param resolver The resolver function to use as the primary strategy.
 */
void snd_syscall_set_resolver(snd_syscall_resolver_t resolver);

/**
 * @brief Adds a fallback syscall resolution strategy to the pipeline.
 * @param resolver The fallback resolver function to add.
 * @retval SND_OK On success.
 * @retval SND_STATUS_SYSCALL_PIPELINE_EXHAUSTED If the pipeline is full.
 */
snd_status_t snd_syscall_add_resolver(snd_syscall_resolver_t resolver);

/**
 * @brief Sets the syscall invoker used by the pipeline.
 * @param invoker Invoker function, or NULL to clear it.
 */
void snd_syscall_set_invoker(snd_syscall_invoker_t invoker);

/**
 * @brief Sets the gadget finder used for indirect invocation.
 * @param finder Gadget finder function, or NULL to clear it.
 */
void snd_syscall_set_gadget_finder(snd_syscall_gadget_finder_t finder);

/**
 * @brief Sets the spoof finder used for spoofed invocation.
 * @param finder Spoof finder function, or NULL to clear it.
 */
void snd_syscall_set_spoof_finder(snd_syscall_gadget_finder_t finder);

/**
 * @brief Core resolution entrypoint (evaluates the internal fallback chain
 * automatically).
 *
 * @param func_hash The hash of the target syscall function name.
 * @param entry_out Pointer to the entry structure to populate.
 * @retval SND_OK On success.
 * @retval SND_STATUS_NULL_POINTER If @p entry_out is NULL.
 * @retval SND_STATUS_NTDLL_NOT_INITIALIZED If the clean NTDLL base is not
 * initialized.
 * @retval SND_STATUS_RESOLVER_NOT_INITIALIZED If no resolver is configured.
 * @retval SND_STATUS_SSN_NOT_FOUND If every configured resolver fails.
 * @retval Any error returned by `snd_ntdll_get_clean_base`,
 * `snd_syscall_resolver_t`, `snd_syscall_gadget_finder_t`, or the configured
 * spoof finder.
 */
snd_status_t snd_syscall_resolve(DWORD func_hash, snd_syscall_entry_t *entry_out);

/**
 * @brief Resolves and invokes a syscall through the configured pipeline.
 * @param sys_hash Hash of the target syscall name.
 * @param args Arguments passed to the configured invoker.
 * @param out_nt_status Receives the raw NTSTATUS returned by the invoker.
 * @retval SND_OK If the syscall was dispatched.
 * @retval SND_STATUS_SYSCALL_INVOKER_NOT_INITIALIZED If no syscall invoker is
 * configured.
 * @retval SND_STATUS_SSN_NOT_FOUND If syscall resolution fails.
 * @retval Any error returned by `snd_syscall_resolve`.
 */
snd_status_t snd_syscall_invoke(DWORD sys_hash, snd_syscall_args_t *args, NTSTATUS *out_nt_status);

/**
 * @brief Invokes a syscall directly through the architecture-specific ASM stub.
 * @param args Pointer to the syscall arguments structure.
 * @retval NTSTATUS_VALUE The raw NTSTATUS returned by the syscall.
 */
extern NTSTATUS snd_syscall_direct_invoke_asm(snd_syscall_args_t *args);

/**
 * @brief Invokes a syscall through a discovered NTDLL transition gadget.
 * @param args Pointer to the syscall arguments structure.
 * @retval NTSTATUS_VALUE The raw NTSTATUS returned by the syscall.
 */
extern NTSTATUS snd_syscall_indirect_invoke_asm(snd_syscall_args_t *args);

/**
 * @brief Invokes a syscall through a discovered gadget with spoofed metadata.
 * @param args Pointer to the syscall arguments structure.
 * @retval NTSTATUS_VALUE The raw NTSTATUS returned by the syscall.
 */
extern NTSTATUS snd_syscall_spoofed_invoke_asm(snd_syscall_args_t *args);

SND_END_EXTERN_C

#endif // SND_PRIMITIVES_SYSCALLS_COMMON_H

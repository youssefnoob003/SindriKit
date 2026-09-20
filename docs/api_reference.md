# SindriKit API Reference

Canonical catalog of the **public C API** under `include/sindri/`. Signatures and types are taken from the headers. Behavior notes call out contracts that matter at the call site (stages, DI tables, ownership). Implementation walkthroughs, PoCs, and the generated status-code table live elsewhere — see [See also](#see-also).

There is no public evasion-domain API.

## Table of Contents

- [How to use this reference](#how-to-use-this-reference)
- [Include map](#include-map)
- [Conventions](#conventions)
- [Status](#status)
- [Common](#common)
  - [Macros](#macros)
  - [Memory helpers](#memory-helpers)
  - [Strings](#strings)
  - [Buffers](#buffers)
  - [Hashing](#hashing)
  - [Debug](#debug)
  - [Opcodes](#opcodes)
- [Primitives](#primitives)
  - [DI tables](#di-tables)
  - [Backend instances](#backend-instances)
  - [Files](#files)
  - [Object manager](#object-manager)
  - [FFI](#ffi)
  - [Heaven's Gate](#heavens-gate)
  - [Syscalls](#syscalls)
- [Parsers](#parsers)
  - [PE](#pe)
  - [COFF](#coff)
  - [Env](#env)
- [Loaders](#loaders)
  - [PE loader](#pe-loader)
  - [COFF loader](#coff-loader)
- [Injection](#injection)
  - [Shared context](#shared-context)
  - [Classic](#classic)
  - [APC](#apc)
- [Internal NT layouts](#internal-nt-layouts)
- [See also](#see-also)

---

## How to use this reference

| You want | Look here |
|---|---|
| Types, signatures, required fields | This file |
| Every `SND_STATUS_*` hex value | [status_codes.md](status_codes.md) (generated) |
| Why DI tables exist | [dependency_injection.md](architecture/dependency_injection.md) |
| Stage machines | [state_machines.md](architecture/state_machines.md) |
| Copy-paste workflows | [basic_usage.md](getting_started/basic_usage.md) |

Functions return `snd_status_t` unless noted. Distinctive codes are named; do not treat those lists as exhaustive — backends and parsers propagate their own facilities.

---

## Include map

| Include | Surface |
|---|---|
| `sindri.h` | Common, status, parsers, primitives, loaders, injection |
| `sindri/common.h` | Buffer, debug, hash, macros, memory, opcodes, string |
| `sindri/status.h` | Core status type, macros, facilities |
| `sindri/primitives.h` | OS API tables, backends, files, syscalls, FFI, Heaven's Gate, thread |
| `sindri/parsers.h` | PE, COFF, env |
| `sindri/loaders.h` | PE and COFF loaders |
| `sindri/injection.h` | Classic, APC, and hijack injection |

Prefer the smallest umbrella that matches the domain. `sindri/internal/` is consumed by the engine; it is not a supported application API.

---

## Conventions

- **Dependency injection.** Loaders and injection never call Win32/NT APIs directly. They call through `mem_api`, `mod_api`, `proc_api`, `thread_api`, and `map` tables you assign on the context.
- **Stages.** Engine functions require a specific `stage` and advance it on success. Chains run the full sequence. Wrong stage → `SND_STATUS_INVALID_STAGE`.
- **Zero-copy parsers.** `snd_pe_parser_t` / `snd_coff_parser_t` pointers alias the caller's `snd_buffer_t`. The buffer must outlive the parser.
- **Debug vs release.** With `SND_DEBUG=0`, `snd_status_t` omits file/line/context, `snd_status_to_string` / stage-to-string helpers return `""`, and debug prints compile out.
- **`SND_SHUFFLE_START` / `SND_SHUFFLE_END`.** Mutator markers. Empty in normal builds.

---

## Status

Headers: `sindri/status.h` → `sindri/status/core.h`, `sindri/status/facility.h`. Domain codes live in `*/status.h` next to each domain.

### Status object

```c
typedef struct _SND_STATUS {
    snd_status_code_t code;   /* int; packed facility + local code */
    int               os_error; /* GetLastError or NTSTATUS when set */
#if SND_DEBUG
    const char *file;
    int         line;
    char        context[SND_MAX_CTX_LEN]; /* 128 */
#endif
} snd_status_t;
```

| Macro | Meaning |
|---|---|
| `SND_MAKE_STATUS(fac, code)` | `((fac) << 16) \| (code & 0xFFFF)` |
| `SND_STATUS_FACILITY(code)` / `SND_STATUS_LOCAL_CODE(code)` | Unpack |
| `SND_SUCCEEDED(x)` / `SND_FAILED(x)` | Compare `x.code` to `SND_SUCCESS` |

Facilities: `SND_FACILITY_GENERIC`, `CLI`, `FILE`, `CONTEXT_MACHINES`, `PARSER_PE`, `PARSER_COFF`, `PARSER_ENV`, `LOADER_PE`, `LOADER_COFF`, `SYSCALL`, `PRIMITIVES`.

### Status macros

| Macro | Effect |
|---|---|
| `SND_OK` | Success object |
| `SND_ERR(code)` | Framework error |
| `SND_ERR_CTX(code, fmt, …)` | Error + formatted context (debug only) |
| `SND_ERR_W32(code)` / `SND_ERR_W32_CTX` | Attach `GetLastError()` |
| `SND_ERR_NT(code, nt)` / `SND_ERR_NT_CTX` | Attach NTSTATUS |
| `SND_CHECK_NULL(p1[, …])` | Up to 5 pointers; returns `SND_STATUS_NULL_POINTER` |
| `SND_TRY(expr)` | Evaluate `snd_status_t` expr; return on failure |

### Core status codes

Defined in `sindri/status/core.h`. Full catalog: [status_codes.md](status_codes.md).

| Code | Typical meaning |
|---|---|
| `SND_SUCCESS` | No error |
| `SND_ERROR_GENERIC` | Unclassified |
| `SND_STATUS_NULL_POINTER` | Required pointer is NULL |
| `SND_STATUS_INVALID_PARAMETERS_COMBINATION` | Contradictory arguments |
| `SND_STATUS_UNSUPPORTED` | Not available in this build |
| `SND_STATUS_ARCH_MISMATCH` | Host vs payload bitness |
| `SND_STATUS_TOO_MANY_ARGUMENTS` | Count exceeds a hard cap (Heaven's Gate: 6) |
| `SND_STATUS_MISSING_COMMAND_LINE_ARGS` / `INVALID_COMMAND_LINE_ARG` | CLI |
| `SND_STATUS_FILE_*` | Path, open, size, read, alloc |
| `SND_STATUS_INVALID_STAGE` / `CORRUPTED_STAGE` | State machines |

### Status strings

```c
const char *snd_status_to_string(snd_status_t status);
void        snd_status_print(snd_status_t status);
```

`snd_status_to_string` dispatches by facility — `snd_generic_status_to_string`, `snd_cli_status_to_string`, `snd_file_status_to_string`, `snd_context_machines_status_to_string`, `snd_pe_parser_status_to_string`, `snd_coff_parser_status_to_string`, `snd_env_parser_status_to_string`, `snd_ldr_pe_status_to_string`, `snd_ldr_coff_status_to_string`, `snd_syscall_status_to_string`, `snd_prim_status_to_string`. Empty when `SND_DEBUG=0`. `snd_status_print` emits via `SND_DEBUG_PRINT` (no-op when debug is off).

---

## Common

Include `sindri/common.h`. Most of this surface is header-only.

### Macros

`sindri/common/macros.h`

| Macro | Role |
|---|---|
| `SND_BEGIN_EXTERN_C` / `SND_END_EXTERN_C` | C++ `extern "C"` |
| `SND_FORCE_INLINE` | Forced inline |
| `SND_MAX` / `SND_MIN` | Ternary min/max |
| `SND_IS_ARCH_COMPATIBLE(is_64bit)` | Host vs image bitness (`_WIN64` vs `_WIN32`) |

### Memory helpers

`sindri/common/memory.h` — CRT-free pointer math, bounds, `memcpy`/`memset` replacements.

| Symbol | Role |
|---|---|
| `SND_PTR_ADD` / `SND_PTR_SUB` | Byte offset |
| `SND_PTR_DELTA(target, preferred)` | Signed delta (`LONG_PTR`) |
| `SND_PTR_DIFF` / `SND_PTR_BEFORE` / `SND_PTR_AFTER` | Distance / order |
| `SND_LOW_WORD` / `SND_HIGH_WORD` | 16-bit split |
| `SND_ALIGN_DOWN` / `SND_ALIGN_UP` | Alignment (`SND_ALIGN_UP` can overflow) |
| `SND_RANGE_EXCEEDS` / `SND_IN_BOUNDS` | Overflow-safe range tests |
| `SND_ADD_OVERFLOWS_DWORD` / `_SIZET` | Add overflow |
| `SND_MUL_OVERFLOWS_DWORD` / `_SIZET` | Mul overflow |
| `SND_MAX_SIZE_T` / `SND_PAGE_SIZE` (`0x1000`) | Limits |
| `snd_memory_bounds_check(total, offset, size)` | `1` if `[offset, offset+size)` ⊆ `[0, total)` |
| `snd_memory_ptr_bounds_check(base, total, ptr, size)` | Same for pointers; `0` if `base`/`ptr` NULL or `ptr < base` |
| `snd_memzero` / `snd_memcpy` | Volatile zero; byte copy. NULL dest/src is a no-op |

`SND_MAX_DWORD` is `0xFFFFFFFF` in `sindri/internal/windows/types.h`.

### Strings

`sindri/common/string.h` — bounded, always-terminating copies.

| ASCII | Wide |
|---|---|
| `snd_strnlen` | `snd_wcsnlen` |
| `snd_strncpy` / `snd_strncat` | `snd_wcsncpy` / `snd_wcsncat` |
| `snd_strnchr` | — |
| `snd_strncmp` / `snd_strnicmp` | `snd_wcsnicmp` |
| `snd_atou32_bounded` → `bool` | `snd_ascii_to_wide` |
| | `snd_wcsn_env_size` — byte size of a double-null env block, or `0` |

### Buffers

`sindri/common/buffer.h`

```c
typedef void (*snd_free_cb)(snd_buffer_t *buf);
typedef struct snd_buffer_s {
    void       *data;
    size_t      size;
    snd_free_cb free_routine;
} snd_buffer_t;

void snd_buffer_init(snd_buffer_t *buf, void *data, size_t size, snd_free_cb free_routine);
void snd_buffer_free(snd_buffer_t *buf); /* invoke free_routine, then zero the struct */
int  snd_buffer_bounds_check(const snd_buffer_t *buf, size_t offset, size_t size);
```

`snd_buffer_init` does not allocate. `snd_buffer_free` is a no-op without a callback. Bounds check fails if `buf`, `data`, or `size` is empty.

Allocator-matched callbacks (declared in `sindri/primitives/memory.h`, installed by file backends):

| Callback | Releases with |
|---|---|
| `snd_buffer_free_win` | `HeapFree` |
| `snd_buffer_free_nt` | `NtFreeVirtualMemory` |
| `snd_buffer_free_sys` | syscall `NtFreeVirtualMemory` |

### Hashing

`sindri/common/hash.h` — algorithm selected at CMake configure time. Compile-time constants: `sindri_hashes.h`.

| Function | Input | Case |
|---|---|---|
| `uint32_t snd_hash(const char *)` | ASCII | Sensitive (export names) |
| `uint32_t snd_hash_lower(const char *)` | ASCII | Lowercased (import DLL names) |
| `uint32_t snd_hash_wide_lower(const wchar_t *)` | UTF-16 | Lowercased (PEB module names) |

### Debug

`sindri/common/debug.h`. CMake `SND_ENABLE_DEBUG` → `SND_DEBUG`. `SND_USE_PRINTF=1` sends output to a `FILE *`; otherwise `OutputDebugStringA` (format cap `SND_DEBUG_MAX_LEN` = 1024).

| Symbol | Role |
|---|---|
| `SND_DEBUG_PRINT(fmt, …)` | Debug log; no-op if `SND_DEBUG=0` |
| `SND_FDEBUG_PRINT(stream, fmt, …)` | Stream when printf path is on |
| `SND_FALLBACK_STR(s)` | `s` in debug, `""` in release |
| `void snd_dump_hex(const void *dat, size_t len, uintptr_t base_off)` | Hex+ASCII; silent if debug off |

### Opcodes

`sindri/common/opcodes.h` — byte constants for scanners (syscall/gadget), not a calling API: `SND_REL32_DISP_SIZE`, `SND_OPCODE_SYSCALL_*`, `SND_OPCODE_RET`, `SND_OPCODE_JMP_INDIRECT_*`, `SND_OPCODE_MOV_EAX_IMM32`, `SND_OPCODE_REX_W_R`, `SND_MODRM_R10_RCX`.

---

## Primitives

Include `sindri/primitives.h`. Tables are defined in `sindri/primitives/os_api.h`. Instances are `extern const` in the domain headers.

### DI tables

All callbacks are `WINAPI` and return `snd_status_t` unless noted.

#### `snd_memory_api_t` — local VA

| Field | Signature |
|---|---|
| `alloc` | `(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protect, LPVOID *out_address)` |
| `free` | `(LPVOID address, SIZE_T size, DWORD free_type)` |
| `protect` | `(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect)` |

`address` NULL lets the OS choose. NT/sys `free` with `size == 0` is success. Primitive codes: `SND_STATUS_ALLOC_FAILED`, `FREE_FAILED`, `PROTECT_FAILED`.

#### `snd_module_api_t` — load / EAT

| Field | Signature |
|---|---|
| `load_library` | `(const char *module_name, HMODULE *out_module)` |
| `get_proc_address` | `(HMODULE, const char *proc_name, FARPROC *)` |
| `get_module_base` | `(const wchar_t *module_name, PVOID *out_base)` — already-loaded only |
| `get_proc_address_hash` | `(HMODULE, DWORD proc_hash, FARPROC *)` |
| `get_module_base_hash` | `(DWORD module_hash, PVOID *out_base)` |

`snd_mod_win` leaves both hash callbacks **NULL**. `snd_mod_nt` PEB-walks before `LdrLoadDll`. Codes: `SND_STATUS_MODULE_LOAD_FAILED`, `PROC_RESOLVE_FAILED`, `MODULE_BASE_GET_FAILED`, `MODULE_NOT_FOUND` (NT/PEB).

`snd_module_resolver_cb` is the same type as `get_module_base` and is what PE export forwarders take. Hash resolvers use `snd_module_resolver_hash_cb`.

#### `snd_mapping_api_t` — named sections

| Field | Signature |
|---|---|
| `open` | `(const wchar_t *section_name, HANDLE *out_handle)` |
| `view` | `(HANDLE section, PVOID *out_base, SIZE_T *out_size)` — read-only |
| `close` | `(HANDLE)` — view remains valid (section semantics) |

Win32 `open` wants Win32/DOS names. NT/sys accept Object Manager paths (`\KnownDlls\ntdll.dll`). Codes: `SND_STATUS_MAPPING_OPEN_FAILED`, `MAPPING_VIEW_FAILED`, `MAPPING_HANDLE_CLOSE_FAILED`.

#### `snd_process_api_t` — remote process

| Field | Role |
|---|---|
| `create_process_params` / `free_process_params` | RTL_USER_PROCESS_PARAMETERS (NT/sys) |
| `create_process` | `(const snd_process_api_t *api, image_path, command_line, HANDLE *out_process, HANDLE *out_thread)` — suspended |
| `open_process` | `(DWORD pid, DWORD desired_access, HANDLE *out)` |
| `alloc_remote` / `free_remote` / `write_remote` / `protect_remote` | Remote VA; alloc uses OS-chosen base |
| `create_remote_thread` | `(process, start, param, HANDLE *out_thread)` |
| `terminate_process` | `(process, exit_code)` — used to abort hijack-created targets |
| `close_handle` | Process or thread handle |

NT/sys create via `NtCreateUserProcess`, with parameters built by the table's `create_process_params`; Win32 via `CreateProcessW`. To customize parameter building, supply your own `snd_process_api_t` with `create_process_params` / `free_process_params` callbacks.

Codes include `SND_STATUS_PROCESS_CREATE_FAILED`, `PROCESS_TERMINATE_FAILED`, `PROCESS_OPEN_FAILED`, `PROCESS_REMOTE_*`, `THREAD_REMOTE_CREATE_FAILED`, `HANDLE_CLOSE_FAILED`.

#### `snd_thread_api_t`

| Field | Signature |
|---|---|
| `queue_apc` | `(HANDLE thread, PVOID apc_routine, PVOID apc_argument)` |
| `resume_thread` / `suspend_thread` | `(HANDLE thread)` |
| `get_context` | `(HANDLE thread, SND_THREAD_REGISTERS *out_regs)` |
| `set_context` | `(HANDLE thread, const SND_THREAD_REGISTERS *in_regs)` |
| `close_handle` | `(HANDLE)` |

`SND_THREAD_REGISTERS` is the portable machine-context projection (`ip`, `sp`, `cx`, `dx`, `rflags`) defined in `sindri/primitives/thread.h`. Entry-frame ABI policy (alignment, register-arg marshalling, EFLAGS) lives natively in the Hijack engine. Backends map the portable registers 1:1 to the native `CONTEXT` using `sindri/internal/windows/context.h`.

Codes: `SND_STATUS_THREAD_QUEUE_FAILED`, `THREAD_RESUME_FAILED`, `THREAD_SUSPEND_FAILED`, `THREAD_GET_CONTEXT_FAILED`, `THREAD_SET_CONTEXT_FAILED`.

### Backend instances

| Table | Win32 | NT (PEB + EAT) | Syscall |
|---|---|---|---|
| Memory | `snd_mem_win` | `snd_mem_nt` | `snd_mem_sys` |
| Modules | `snd_mod_win` | `snd_mod_nt` | — |
| Mapping | `snd_map_win` | `snd_map_nt` | `snd_map_sys` |
| Process | `snd_proc_win` | `snd_proc_nt` | `snd_proc_sys` |
| Thread | `snd_thread_win` | `snd_thread_nt` | `snd_thread_sys` |
| Files | `snd_file_win` | `snd_file_nt` | `snd_file_sys` |

`*_sys` backends require [syscall bootstrap](#syscalls) (`snd_ntdll_set_clean` + resolver/invoker). `SND_USE_DEFAULTS=ON` still requires a clean NTDLL base.

### Files

`sindri/primitives/files.h`

```c
typedef snd_status_t (WINAPI *snd_file_load_cb)(const char *path, snd_buffer_t *out_buffer);
typedef struct { snd_file_load_cb load; } snd_file_api_t;
```

Reads the whole file into `out_buffer` and installs the matching `snd_buffer_free_*`. Rejects empty files and sizes above `SND_MAX_DWORD`.

| Backend | Path rule |
|---|---|
| `snd_file_win` | Relative or absolute |
| `snd_file_nt` / `snd_file_sys` | Absolute DOS path only (`C:\…`); prefixed as `\??\` |

### Object manager

`sindri/primitives/object_manager.h`

```c
#define SND_TARGET_KNOWNDLLS_DIR  /* L"\\KnownDlls\\" (x64) or L"\\KnownDlls32\\" (x86) */

snd_status_t snd_om_knowndll_map(
    const snd_mapping_api_t *config, const wchar_t *dll_name, PVOID *out_base_address);
```

Prepends `SND_TARGET_KNOWNDLLS_DIR` to `dll_name` (file name only, e.g. `L"ntdll.dll"`). Use `&snd_map_nt` or `&snd_map_sys` — Win32 mapping cannot open `\KnownDlls`. Missing `open`/`view` → `SND_STATUS_OM_NOT_INITIALIZED`. Closes the section handle after mapping; the mapped view stays valid, and a close failure is logged but not fatal.

### FFI

`sindri/primitives/ffi.h`

```c
UINT_PTR snd_ffi_execute(PVOID pFunctionAddress, DWORD dwArgCount, const UINT_PTR *pArgs);
```

Returns the callee’s return value as `UINT_PTR`, or `0` if the function is NULL, or if `dwArgCount > 0` and `pArgs` is NULL. No argument-count cap in the C wrapper. Callee-saved registers are preserved by the ASM bridge (`snd_ffi_bridge_x64` / `_x86` — not public).

### Heaven's Gate

`sindri/primitives/heavens_gate.h`

```c
#define SND_HG_MAX_ARGS 6
BOOL         snd_is_wow64(void); /* x86 TEB fs:[0xC0]; always FALSE on _WIN64 */
snd_status_t snd_hg_execute_64(ULONGLONG pFunctionAddress, DWORD dwArgCount,
                               const ULONGLONG *pArgs, ULONGLONG *pResult);
```

Far return to CS `0x33`. `pResult` may be NULL. Failures: `NULL_POINTER` (address 0), `TOO_MANY_ARGUMENTS` (> 6), `INVALID_PARAMETERS_COMBINATION` (count > 0, `pArgs` NULL), `ARCH_MISMATCH` (native x64, non-WoW64, or 32-bit OS).

### Syscalls

`sindri/primitives/syscalls.h`. Pipeline lives in `src/primitives/execution/syscalls/`. Prefer `snd_syscall_invoke`; ASM invokers are the functions you register, not typical call sites.

#### Constants and types

| Macro | Value |
|---|---|
| `SND_MAX_SYSCALLS` | 500 (sort-resolver cache) |
| `SND_MAX_SYS_NAME_LEN` | 256 |
| `SND_MAX_INTERNAL_STRATEGIES` | 4 (resolver chain depth) |
| `SND_SPOOF_FRAME_MIN_SIZE` / `MAX_SIZE` | 120 / 496 — must match ASM stack |

```c
typedef struct {
    PVOID pAddress;       /* stub (scan resolver) */
    DWORD dwHash;
    WORD  wSystemCall;    /* SSN; sort resolver fills only this */
    PVOID pSyscallAddr;   /* indirect gadget */
    PVOID pSpoofAddr;
    DWORD dwSpoofFrameSize;
} snd_syscall_entry_t;

typedef struct {
    WORD  ssn;
    PVOID arg1; /* … arg11 — x64: arg1→R10, arg2–4 RDX/R8/R9, rest stack */
    PVOID sys_addr;        /* trailing so arg offsets stay stable */
    PVOID spoof_addr;
    DWORD spoof_frame_size;
} snd_syscall_args_t;

typedef snd_status_t (*snd_syscall_resolver_t)(DWORD func_hash, snd_syscall_entry_t *entry_out);
typedef NTSTATUS     (*snd_syscall_invoker_t)(snd_syscall_args_t *args);
typedef snd_status_t (*snd_syscall_gadget_finder_t)(snd_syscall_entry_t *entry);
```

#### Pipeline

| Function | Role |
|---|---|
| `void snd_syscall_set_resolver(resolver)` | Primary strategy; **resets** the chain to one entry |
| `snd_status_t snd_syscall_add_resolver(resolver)` | Fallback; `SYSCALL_PIPELINE_EXHAUSTED` if full |
| `void snd_syscall_set_invoker(invoker)` | Direct / indirect / spoofed ASM |
| `void snd_syscall_set_gadget_finder(finder)` | Required for indirect (and spoofed) |
| `void snd_syscall_set_spoof_finder(finder)` | Required for spoofed |
| `snd_status_t snd_syscall_resolve(hash, entry_out)` | First successful resolver; then gadget/spoof finders if set |
| `snd_status_t snd_syscall_invoke(hash, args, NTSTATUS *out)` | Resolve + configured invoker |
| `void snd_syscall_cache_enable(BOOL enabled)` | Opt-in memoization of resolved entries (256, cleared on disable/`set_resolver`); bypassed when a spoof finder is set |

`snd_syscall_resolve` / `invoke` distinctive codes: `SND_STATUS_NTDLL_NOT_INITIALIZED` (no clean NTDLL), `RESOLVER_NOT_INITIALIZED`, `SSN_NOT_FOUND`, `SYSCALL_INVOKER_NOT_INITIALIZED`, `GADGET_NOT_FOUND`, `SPOOF_GADGET_NOT_FOUND`.

Built-ins:

| Symbol | Kind |
|---|---|
| `snd_syscall_resolve_ssn_scan` | Stub-byte SSN + neighbor fallback; uses **clean** NTDLL |
| `snd_syscall_resolve_ssn_sort` | EAT sort table on **clean** NTDLL |
| `snd_syscall_find_gadget_scan` | `syscall; ret` in **active** NTDLL |
| `snd_syscall_find_spoof_scan` | Fat-frame spoof gadget |
| `snd_syscall_direct_invoke_asm` | Inline `syscall` / `sysenter` |
| `snd_syscall_indirect_invoke_asm` | Jump to `sys_addr` |
| `snd_syscall_spoofed_invoke_asm` | Indirect + spoofed return |

ASM invokers return raw `NTSTATUS` (`STATUS_INVALID_PARAMETER` / `0xC000000D` if required gadget fields are NULL). They are not a supported public call pattern except as `snd_syscall_set_invoker` arguments.

#### `SND_USE_DEFAULTS`

Default **0**. When **1**:

| Macro | Default |
|---|---|
| `SND_SYSCALL_INVOKER_DEFAULT` | `snd_syscall_indirect_invoke_asm` |
| `SND_SYSCALL_GADGET_FINDER_DEFAULT` | `snd_syscall_find_gadget_scan` |
| `SND_SYSCALL_SPOOF_FINDER_DEFAULT` | `snd_syscall_find_spoof_scan` |
| `SND_SYSCALL_RESOLVER_DEFAULT` | `snd_syscall_resolve_ssn_scan` |

Otherwise all four are `NULL`. Defaults are macros so unused invokers/scanners are not pulled into CRT-less binaries.

Typical bootstrap: `snd_om_knowndll_map` → `snd_ntdll_set_clean` → `set_resolver` (+ optional `add_resolver`) → `set_invoker` → gadget/spoof finders as required → `snd_syscall_invoke(SND_HASH_…, &args, &nt)`.

---

## Parsers

Include `sindri/parsers.h`, or `sindri/parsers/pe.h` / `coff.h` / `env.h`.

### PE

Zero-copy. `snd_pe_parse(source, is_mapped, parser)` validates DOS `MZ`, NT `PE\0\0`, optional magic `0x10B` / `0x20B`. If `is_mapped` and `source->size == SND_SYS_DLL_SIZE_DEFAULT` (`0x1000`), `parser->source.size` is expanded to `SizeOfImage` (see caution comment in `parser.h`).

```c
typedef struct {
    snd_buffer_t source;
    PSND_IMAGE_DOS_HEADER dos;
    union { PSND_IMAGE_NT_HEADERS32 nt32; PSND_IMAGE_NT_HEADERS64 nt64; } nt;
    PSND_IMAGE_SECTION_HEADER section_head;
    BYTE *string_table; /* raw files only; NULL when mapped */
    BOOL is_64bit, is_dll, is_mapped;
    SIZE_T lfanew;
    DWORD sections_count, imports_rva, import_size;
} snd_pe_parser_t;
```

| Symbol | Contract |
|---|---|
| `SND_PE_GET_NT_FIELD(parser, field)` | Optional-header field without bitness branch |
| `snd_pe_get_directory(parser, index, dir_out)` | Data directory by index |
| `PVOID snd_pe_rva_to_ptr(parser, rva, size)` | Mapped: `base+rva`. Raw: section translation. NULL if OOB |
| `PVOID snd_pe_get_entry_point(parser)` | `AddressOfEntryPoint` or NULL |
| `PVOID snd_pe_get_tls_callbacks(parser)` | Callback array or NULL |
| `DWORD snd_pe_get_page_protection_flags(pe, page_offset)` | `SND_PAGE_*` from section chars |
| `SND_PE_MIN_FILE_ALIGNMENT` | 512 |

**Exports** (`exports.h`). Forwarder cap `SND_FWD_MAX_DEPTH` = 4. Both name and hash resolvers take the **same** `snd_module_resolver_cb` (wide name). `NULL` resolver fails forwarders (`SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED`). Ordinals via `SND_MAKEINTRESOURCE`.

```c
snd_status_t snd_pe_get_export_address(const snd_pe_parser_t *, const char *func_name,
                                       FARPROC *out, snd_module_resolver_cb resolver);
snd_status_t snd_pe_get_export_address_hash(const snd_pe_parser_t *, DWORD func_hash,
                                            FARPROC *out, snd_module_resolver_cb resolver);
typedef BOOL (*snd_pe_export_enum_cb)(const char *func_name, WORD ordinal, PVOID addr, PVOID user_ctx);
snd_status_t snd_pe_enumerate_exports(const snd_pe_parser_t *, snd_pe_export_enum_cb, PVOID user_ctx);
```

Wire forwarders with `snd_peb_get_module_base`.

**Imports** (`imports.h`)

```c
typedef struct {
    const char *name;   /* NULL if ordinal */
    WORD        ordinal;
    BOOL        is_ordinal;
    PVOID       iat_slot;
} snd_pe_import_thunk_t;
```

| Function | Notes |
|---|---|
| `snd_pe_get_import_descriptor(parser, index, out_desc)` | `*out_desc == NULL` at terminator |
| `snd_pe_get_import_name(parser, desc, out_name)` | DLL name |
| `snd_pe_get_import_thunk(parser, desc, thunk_index, out)` | `iat_slot`/`name` NULL at terminator |

`SND_PE_SNAP_BY_ORDINAL32` / `64` test the high bit; `SND_PE_ORDINAL` masks 16 bits.

**Relocations** (`relocations.h`)

```c
typedef struct {
    WORD  type;      /* HIGHLOW / DIR64 / ABSOLUTE */
    DWORD patch_rva;
    PVOID patch_ptr; /* NULL for ABSOLUTE padding */
} snd_pe_reloc_entry_t;

snd_status_t snd_pe_get_reloc_block(parser, SIZE_T *cursor, const SND_IMAGE_BASE_RELOCATION **block, DWORD *count);
snd_status_t snd_pe_get_reloc_entry(parser, const SND_IMAGE_BASE_RELOCATION *block, DWORD index, snd_pe_reloc_entry_t *out);
```

Set `*cursor = 0` on first `get_reloc_block`; `*block == NULL` at end.

**Sections** (`section.h`) — used by the PE loader; still on the public parser umbrella:

| Function | Role |
|---|---|
| `snd_pe_section_name(parser, section, buf, size)` | Including COFF long names |
| `snd_pe_section_copy_size` | `min(SizeOfRawData, VirtualSize)` (raw copy) |
| `snd_pe_section_loaded_size` | `VirtualSize` or `SizeOfRawData` |
| `SND_PE_MAX_SECTION_NAME_LEN` | 256 |

### COFF

Zero-copy object files.

```c
typedef struct {
    snd_buffer_t source;
    PSND_IMAGE_FILE_HEADER file_header;
    PSND_IMAGE_SECTION_HEADER section_head;
    PSND_IMAGE_SYMBOL symbol_table;
    DWORD symbol_count;
    BYTE *string_table;
    DWORD string_table_size;
    BOOL  is_64bit;
    DWORD sections_count;
} snd_coff_parser_t;

snd_status_t snd_coff_parse(const snd_buffer_t *buf, snd_coff_parser_t *parser);
```

| Function | Role |
|---|---|
| `snd_coff_get_symbol_name` / `snd_coff_get_section_name` | Inline ≤8 chars or string table → caller buffer |
| `PVOID snd_coff_raw_to_ptr(parser, raw_offset, size)` | File offset → pointer or NULL |
| `PSND_IMAGE_SYMBOL snd_coff_get_symbol_by_index` | Raw slot; aux records occupy indices |
| `snd_coff_find_symbol_by_name(parser, name, name_len, symbol_out, index_out)` | Exact match; `index_out` optional |
| `snd_coff_decode_symbol` | Classify LOCAL / BSS / IMPORT (`__imp_DLL$Func`) / OTHER |
| `snd_coff_get_relocations(parser, section, relocs_out, count_out)` | `SND_OK` even when count is 0 |
| `snd_coff_get_relocation_patch_size(is_64bit, type, size_out)` | Patch width in bytes |
| `SND_COFF_MAX_SYMBOL_LEN` | 4096 |

```c
typedef enum {
    SND_COFF_SYM_TYPE_LOCAL, SND_COFF_SYM_TYPE_BSS,
    SND_COFF_SYM_TYPE_IMPORT, SND_COFF_SYM_TYPE_OTHER
} snd_coff_sym_type_t;

typedef struct { char dll_name[128]; char func_name[128]; BOOL is_imp; } snd_coff_import_info_t;
typedef struct {
    snd_coff_sym_type_t type;
    snd_coff_import_info_t import; /* valid if IMPORT */
    SIZE_T bss_size;               /* valid if BSS */
} snd_coff_decoded_sym_t;
```

### Env

PEB / NTDLL. Layouts: `sindri/internal/nt/peb.h`.

```c
PSND_PEB     snd_peb_get_local(void);              /* gs:0x60 / fs:0x30 / x18:0x60 */
PVOID        snd_env_get_wow32_reserved(void);     /* x86 fs:0xC0; NULL elsewhere */
snd_status_t snd_peb_get_module_base(const wchar_t *name, PVOID *out_base);
snd_status_t snd_peb_get_module_base_hash(DWORD module_hash, PVOID *out_base);
snd_status_t snd_env_get_process_params(const PSND_PEB peb, PSND_RTL_USER_PROCESS_PARAMETERS *out);
snd_status_t snd_env_get_command_line(const PSND_PEB peb, SND_UNICODE_STRING **out);
```

`peb == NULL` on the env getters means “local PEB”. Module walk is case-insensitive on `BaseDllName`, capped by `MAX_ITERATIONS` (500). Hash lookup uses `sindri_hashes.h` (e.g. `SND_HASH_NTDLL_DLL`).

**NTDLL registry** (`ntdll.h`) — used by the syscall pipeline:

| Function | Side |
|---|---|
| `snd_ntdll_set_clean(PVOID clean_base)` | Parse and store unhooked image (`snd_pe_parse` mapped) |
| `snd_ntdll_get_clean_parser` / `snd_ntdll_get_clean_base` / `snd_ntdll_get_clean_export` | Require `set_clean`; else `SND_STATUS_NTDLL_CLEAN_NOT_INITIALIZED` |
| `snd_ntdll_get_active_parser` / `snd_ntdll_get_active_base` / `snd_ntdll_get_active_export` | Lazy PEB NTDLL |

Export lookups take a compile-time hash. `snd_ntdll_entry_t` `{ base, parser, is_initialized }` is the stored slot.

Env codes include `SND_STATUS_MODULE_NOT_FOUND`, `PEB_GET_FAILED`, `PEB_LDR_NOT_INITIALIZED`, `MODULE_LIST_CORRUPTED`, `PROCESS_PARAMS_NOT_FOUND`.

---

## Loaders

Include `sindri/loaders.h`. Assign `raw_source`, `mem_api`, and `mod_api` before any chain.

### PE loader

`sindri/loaders/pe/engine.h`, `chain.h`.

```c
typedef struct {
    LPVOID local_base, execution_base, entry_point;
    SIZE_T allocated_size;
    LONG_PTR delta_offset; /* execution_base - ImageBase */
} snd_pe_target_t;

typedef enum {
    SND_STAGE_UNINITIALIZED = 0,
    SND_STAGE_PARSED,
    SND_STAGE_MEM_ALLOCATED,
    SND_STAGE_SECTIONS_MAPPED,
    SND_STAGE_RELOCATED,
    SND_STAGE_IMPORTS_RESOLVED,
    SND_STAGE_READY_FOR_EXECUTION,
    SND_STAGE_EXECUTED,
} snd_ldr_pe_stage_t;

typedef struct _snd_ldr_pe_ctx {
    const snd_buffer_t *raw_source;
    snd_pe_parser_t     pe;
    snd_pe_target_t     target;
    snd_ldr_pe_stage_t  stage;
    const snd_memory_api_t *mem_api;
    const snd_module_api_t *mod_api;
} snd_ldr_pe_ctx_t;
```

`execution_base != local_base` means the image was relocated for a remote process. Local execute/detach refuse that configuration (`SND_STATUS_LOCAL_EXECUTION_BLOCKED` / no-op).

**Chain**

| Function | Does |
|---|---|
| `snd_ldr_pe_prepare_image(ctx)` | Parse raw (`is_mapped=FALSE`) → arch check → alloc/copy → relocs → imports → page protect. Stops at `READY_FOR_EXECUTION`. **Does not** run TLS or the entry point. Frees the mapping on failure after alloc |
| `snd_ldr_pe_execute_image(ctx)` | Requires `READY_FOR_EXECUTION` and `local_base == execution_base`. TLS `DLL_PROCESS_ATTACH`, then `DllMain` (DLL; NULL entry is success) or EXE jump |
| `void snd_ldr_pe_detach_image(ctx)` | Requires `EXECUTED` and matching bases. DLL: `DllMain` detach + TLS detach, then free. No-op otherwise |

**Engine** (strict stages)

| Function | Requires | Advances to |
|---|---|---|
| `snd_ldr_pe_allocate_and_copy_image` | `PARSED` | `SECTIONS_MAPPED` (re-parses mapped) |
| `snd_ldr_pe_apply_relocations` | `SECTIONS_MAPPED` | `RELOCATED` |
| `snd_ldr_pe_resolve_imports` | `RELOCATED` | `IMPORTS_RESOLVED` |
| `snd_ldr_pe_apply_memory_protections` | `IMPORTS_RESOLVED` | `READY_FOR_EXECUTION` (also resolves `entry_point`) |
| `snd_ldr_pe_execute_tls_callbacks(ctx, reason)` | ≥ `READY_FOR_EXECUTION` | (void) |
| `snd_ldr_pe_get_proc_address(ctx, name, out)` | ≥ `READY_FOR_EXECUTION` | via `snd_pe_get_export_address` + `mod_api->get_module_base` |
| `void snd_ldr_pe_free_mapped_image(ctx)` | any with `local_base` | toward `PARSED` |

Use `free_mapped_image` after prepare-only or failed loads. Use `detach_image` after a successful **local** `execute_image`.

```c
SND_CALL_EXPORT(ctx, name, signature, status_out, ...)
SND_CALL_EXPORT_RET(ctx, name, signature, status_out, ret_out, ...)
```

Both resolve with `snd_ldr_pe_get_proc_address` and invoke only if `status_out.code == SND_SUCCESS`.

`snd_ldr_pe_stage_to_string` is empty when `SND_DEBUG=0`.

### COFF loader

`sindri/loaders/coff/engine.h`, `chain.h`.

```c
typedef struct {
    LPVOID local_base, execution_base;
    SIZE_T allocated_size;
    LPVOID *section_map;  /* 1-based section index → VA */
    LPVOID *symbol_map;
    LPVOID iat_base, trampolines_base, bss_base;
} snd_coff_target_t;

typedef enum {
    SND_COFF_STAGE_UNINITIALIZED = 0,
    SND_COFF_STAGE_PARSED,
    SND_COFF_STAGE_MEM_ALLOCATED,
    SND_COFF_STAGE_SECTIONS_MAPPED,
    SND_COFF_STAGE_SYMBOLS_RESOLVED,
    SND_COFF_STAGE_RELOCATED,
    SND_COFF_STAGE_READY_FOR_EXECUTION,
    SND_COFF_STAGE_EXECUTED,
} snd_ldr_coff_stage_t;

typedef struct _snd_ldr_coff_ctx {
    const snd_buffer_t  *raw_source;
    snd_coff_parser_t    coff;
    snd_coff_target_t    target;
    snd_ldr_coff_stage_t stage;
    const snd_memory_api_t *mem_api;
    const snd_module_api_t *mod_api;
} snd_ldr_coff_ctx_t;
```

| Function | Role |
|---|---|
| `snd_ldr_coff_load(ctx)` | Parse → copy sections → resolve `MODULE$Func` via `mod_api` → relocs → protect. Leaves `READY_FOR_EXECUTION`. Frees on failure after alloc |
| `snd_ldr_coff_execute_image(ctx, entry_name, bof_args, bof_arg_len)` | Local call of named symbol (e.g. `"go"`) |
| `snd_ldr_coff_allocate_and_copy_sections` | Requires `PARSED` |
| `snd_ldr_coff_resolve_symbols` | Requires `SECTIONS_MAPPED` |
| `snd_ldr_coff_apply_relocations` | Requires `SYMBOLS_RESOLVED` |
| `snd_ldr_coff_apply_memory_protections` | Requires `RELOCATED` |
| `void snd_ldr_coff_free_mapped_image(ctx)` | Release mapping |

`snd_ldr_coff_stage_to_string` follows the same debug rule as the PE helper. x64 distant calls use 16-byte `JMP [RIP+0]` trampolines (`SND_X64_TRAMPOLINE_SIZE`).

---

## Injection

Include `sindri/injection.h`. Classic opens an existing PID. APC creates a suspended process from `target_image_path` and queues an APC on the primary thread. Hijack creates a suspended process and rewrites its initial thread's context before resuming.

### Shared context

`sindri/injection/common/context.h` and `sindri/injection/common/cleanup.h`

```c
typedef enum {
    SND_INJ_STAGE_UNINITIALIZED = 0,
    SND_INJ_STAGE_TARGET_ACQUIRED,
    SND_INJ_STAGE_MEMORY_ALLOCATED,
    SND_INJ_STAGE_PAYLOAD_WRITTEN,
    SND_INJ_STAGE_PROTECTIONS_SET,
    SND_INJ_STAGE_CONTEXT_APPLIED,
    SND_INJ_STAGE_EXECUTED,
} snd_inj_stage_t;

typedef enum {
    SND_INJ_RETURN_NONE = 0,
    SND_INJ_RETURN_GRACEFUL,
} snd_inj_return_policy_t;

typedef struct _snd_inj_ctx_t {
    DWORD  target_pid;          /* classic */
    HANDLE target_process;
    PVOID  remote_base;
    PVOID  remote_entry_point;  /* thread/APC start; else remote_base */
    SIZE_T remote_size;
    HANDLE remote_thread;
    PVOID  remote_arg;          /* remote BOF args (COFF paths) */
    BOOL   owns_target_process; /* cleanup may terminate a created target */

    snd_inj_stage_t stage;

    const snd_buffer_t      *payload;
    const snd_process_api_t *proc_api;
    const snd_thread_api_t  *thread_api;        /* APC, hijack */
    const wchar_t           *target_image_path; /* APC, hijack */
    snd_inj_return_policy_t  return_policy;     /* hijack return handling */
} snd_inj_ctx_t;

const char *snd_inj_stage_to_string(snd_inj_stage_t stage); /* "" if !SND_DEBUG */
void        snd_inj_cleanup(snd_inj_ctx_t *ctx);
```

`snd_inj_cleanup` best-effort closes `remote_thread` through `thread_api` when
available, otherwise through `proc_api`, closes `target_process`, and releases
pre-execution remote memory when `proc_api->free_remote` is available. It does
not release remote memory after successful execution or free loader-local
mappings. It resets the context stage and remote fields. No-op if `ctx` or
`proc_api` is NULL.

| Chain | Required before start |
|---|---|
| Classic | `target_pid`, `proc_api`; `payload` for alloc/write (shell). PE/COFF: loader ctx + `inj_ctx` |
| APC | `target_image_path`, `proc_api`, `thread_api`; `payload` for shell |
| Hijack | `target_image_path`, `proc_api`, `thread_api`; `payload` for shell |

Injection has no domain-specific status header codes; failures are core stage errors plus primitive `PROCESS_*` / `THREAD_*`.

### Common Injection

`sindri/injection/common/target.h`, `staging.h`, `prepare.h`, `cleanup.h`

The technique engines share target acquisition and remote staging through the
common injection layer. Execution-specific engine headers expose only their
technique-specific execution operations.

| Function | Role |
|---|---|
| `snd_inj_open_target` | Open an existing process by PID |
| `snd_inj_create_suspended_target` | Create a suspended target process |
| `snd_inj_alloc_remote` / `snd_inj_alloc_remote_size` | Allocate remote staging memory |
| `snd_inj_write_payload` | Write only the payload buffer |
| `snd_inj_set_protections` | Transition staged memory to execute-read |
| `snd_inj_prepare_pe` / `snd_inj_prepare_coff` | Bake and stage loader payloads for a selected target mode |
| `snd_inj_cleanup` | Best-effort handles, target, and pre-execution remote-memory cleanup |

### Classic

`sindri/injection/classic/engine.h`, `chain.h`

Engine (each step needs the previous stage):

| Function | Callback | Next stage |
|---|---|---|
| `snd_inj_open_target` | `open_process(pid, SND_PROCESS_ALL_ACCESS)` | `TARGET_ACQUIRED` |
| `snd_inj_alloc_remote` | `alloc_remote` `MEM_COMMIT\|RESERVE`, `PAGE_READWRITE`, size = `payload->size` | `MEMORY_ALLOCATED` |
| `snd_inj_write_payload` | `write_remote` | `PAYLOAD_WRITTEN` |
| `snd_inj_set_protections` | `protect_remote` → `SND_PAGE_EXECUTE_READ` | `PROTECTIONS_SET` |
| `snd_inj_classic_execute` | `create_remote_thread` at `remote_entry_point` or `remote_base`, param `NULL` | `EXECUTED` |

```c
snd_status_t snd_inj_classic_shell(snd_inj_ctx_t *ctx);
snd_status_t snd_inj_classic_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx);
snd_status_t snd_inj_classic_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx,
                                  const char *entry_point, void *args, int arg_len);
```

PE/COFF chains parse and map **locally**, relocate against the **remote** base, write the baked image, then execute. Loader ctx needs `raw_source`, `mem_api`, `mod_api`. COFF allocates room for sections plus args and passes the remote args address into the named symbol (`"go"`).

### APC

`sindri/injection/apc/engine.h`, `chain.h`

Same memory stages as classic; target acquisition and execute differ:

| Function | Callback | Next stage |
|---|---|---|
| `snd_inj_create_suspended_target` | `create_process` (suspended) | `TARGET_ACQUIRED` |
| `snd_inj_alloc_remote` / `snd_inj_write_payload` / `snd_inj_set_protections` | same as classic | … |
| `snd_inj_apc_execute` | `thread_api->queue_apc` then `resume_thread` | `EXECUTED` |

```c
snd_status_t snd_inj_apc_shell(snd_inj_ctx_t *ctx);
snd_status_t snd_inj_apc_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx);
snd_status_t snd_inj_apc_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx,
                              const char *entry_point, void *args, int arg_len);
```

### Hijack

`sindri/injection/hijack/engine.h`, `chain.h`

Spawns the target suspended, stages the payload through the shared remote-write
steps, then rewrites the suspended initial thread's context instead of creating
or APC-queueing a thread. No `create_remote_thread` and no APC.

| Function | Callback | Next stage |
|---|---|---|
| `snd_inj_create_suspended_target` | `create_process` (suspended) | `TARGET_ACQUIRED` |
| (shared staging) | `snd_inj_alloc_remote` / `snd_inj_write_payload` / `snd_inj_set_protections` | `PROTECTIONS_SET` |
| `snd_inj_hijack_execute` | `thread_api->get_context` → prepare ABI frame → `write_remote` (stack data) → `set_context` → `resume_thread` | `EXECUTED` |

```c
typedef struct {
    SND_THREAD_REGISTERS registers;
    ULONG_PTR            stack[3];
    SIZE_T               stack_size;
} snd_inj_entry_frame_t;

snd_status_t snd_inj_hijack_prepare_frame(const SND_THREAD_REGISTERS *live, PVOID entry, PVOID return_thunk,
                                           ULONG_PTR arg1, ULONG_PTR arg2, snd_inj_entry_frame_t *out);
snd_status_t snd_inj_hijack_execute(snd_inj_ctx_t *ctx, ULONG_PTR arg1, ULONG_PTR arg2);

snd_status_t snd_inj_hijack_shell(snd_inj_ctx_t *ctx);
snd_status_t snd_inj_hijack_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx);
snd_status_t snd_inj_hijack_coff(snd_ldr_coff_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx, const char *entry_point,
                                 void *args, int arg_len);
```

The entry frame is prepared by `snd_inj_hijack_prepare_frame`: `ip := entry`,
the architecture-specific `sp` is selected, `rflags` is sanitized, and the
same logical call `(arg1, arg2)` is represented in the returned frame. x64 uses
`cx`/`dx`; x86 uses stack words. The returned stack data is written to the
target before `set_context` only when the frame has content; a frameless hijack
(with `SND_INJ_RETURN_NONE` and zero arguments) performs no remote write, so a
process table without `write_remote` is still usable. `write_remote` is required
only when data must be written. `SND_INJ_RETURN_NONE` means the payload must not
return; `SND_INJ_RETURN_GRACEFUL` makes the engine resolve a target-compatible
thread-exit address.

---

## Internal NT layouts

Not part of `sindri.h`. Include `sindri/internal/nt.h` only when you must talk NT types (env parser, custom process-params callbacks).

| Header | Contents |
|---|---|
| `nt/base.h` | `SND_NT_SUCCESS`, `SND_OBJ_CASE_INSENSITIVE`, `SND_InitializeObjectAttributes`, `SND_UNICODE_STRING`, `SND_OBJECT_ATTRIBUTES`, `SND_CLIENT_ID`, `snd_init_unicode_string` |
| `nt/api.h` | NT function-pointer typedefs (`LdrLoadDll`, `Nt*`) |
| `nt/peb.h` | `SND_PEB`, `SND_PEB_LDR_DATA`, `SND_LDR_DATA_TABLE_ENTRY`, `SND_RTL_USER_PROCESS_PARAMETERS`, `SND_CURDIR` |
| `nt/file.h` / `nt/process.h` | File and process NT structures |

---

## See also

- [Getting started](getting_started/README.md) — build flags and bootstrap
- [Status codes](status_codes.md) — generated table
- [Status architecture](architecture/status_system.md)
- [Dependency injection](architecture/dependency_injection.md)
- [State machines](architecture/state_machines.md)
- [Hash manifest](config/hashes_manifest.md)
- Domain internals: [common](common/README.md), [primitives](primitives/README.md), [parsers](parsers/README.md), [loaders](loaders/README.md), [injection](injection/README.md)

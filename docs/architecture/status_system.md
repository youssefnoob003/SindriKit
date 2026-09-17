# The Status System

SindriKit uses a universal **`snd_status_t`** return type instead of bare integers or NULL checks. Failures carry a framework error code and the underlying OS error (Win32 or NTSTATUS) in a single struct.

**Headers:** `include/sindri/status.h`, `include/sindri/status/`
**Implementation:** `src/status/` (generic/CLI/file/core) plus per-domain resolvers such as `src/parsers/pe/status.c` and `src/loaders/pe/status.c`

---

## The `snd_status_t` structure

### Always present

| Field | Type | Purpose |
|---|---|---|
| `code` | `snd_status_code_t` | Framework-level result (success or domain-specific failure) |
| `os_error` | `int` | `GetLastError()` or NTSTATUS captured at failure site |

### DEBUG tier only (`SND_DEBUG=1`)

| Field | Purpose |
|---|---|
| `file` | `__FILE__` of failing macro |
| `line` | `__LINE__` |
| `context` | `char[SND_MAX_CTX_LEN]` — formatted detail (128 bytes incl. NUL) |

When `SND_ENABLE_DEBUG=OFF`, the struct shrinks to `{ code, os_error }` only. String formatting arguments to error macros are compiled out.

---

## DEBUG vs SILENT tiers

Controlled by CMake **`SND_ENABLE_DEBUG`** → preprocessor **`SND_DEBUG`**.

| Tier | CMake | Struct size | String literals in `.rdata` |
|---|---|---|---|
| DEBUG | `SND_ENABLE_DEBUG=ON` | Full | File paths, stage names, `SND_ERR_CTX` format strings |
| SILENT | `SND_ENABLE_DEBUG=OFF` | Minimal | Stripped — production implants only |

> [!CAUTION]
> Plaintext error strings in release binaries are signature surface. Production artifacts must use the SILENT tier.

`SND_USE_PRINTF=ON` (requires DEBUG) routes diagnostics to `stdout` instead of `OutputDebugStringA` and pulls in CRT I/O.

---

## Helper macros

| Macro | Behavior |
|---|---|
| `SND_OK` | Success (`code == SND_SUCCESS`) |
| `SND_SUCCEEDED(x)` / `SND_FAILED(x)` | Test `x.code` |
| `SND_ERR(code)` | Framework error, `os_error = 0` |
| `SND_ERR_CTX(code, fmt, …)` | Error + formatted context (DEBUG only) |
| `SND_ERR_W32(code)` | Error + `GetLastError()` |
| `SND_ERR_W32_CTX(code, fmt, …)` | W32 + context |
| `SND_ERR_NT(code, nt_status)` | Error + NTSTATUS |
| `SND_ERR_NT_CTX(code, nt_status, fmt, …)` | NT + context |

Release builds map `SND_ERR_CTX` / `SND_ERR_W32_CTX` / `SND_ERR_NT_CTX` to the non-context variants — format arguments are not evaluated for side effects in a way that emits strings, but callers should not rely on context formatting in SILENT.

---

## Human-readable output

| Function | DEBUG | SILENT |
|---|---|---|
| `snd_status_to_string(status)` | Description of `code` | Empty string |
| `snd_status_print(status)` | Prints code, context, os_error, file/line | No-op body |

PoCs and development builds use `snd_status_print` on failure paths:

```c
snd_status_t status = snd_ldr_pe_prepare_image(&ctx);
if (SND_FAILED(status)) {
    snd_status_print(status);
    return status.code;
}
```

---

## Architecture overview

The status architecture uses structured facility bitmask encoding to classify status codes, then dispatches each facility to a dedicated string resolver:

```text
snd_status_to_string(status)
        |
        v
SND_STATUS_FACILITY(status.code) --> resolver
        |
        +-- 0x0000 Generic / 0x0003 Context machines -> src/status/generic.c
        +-- 0x0001 CLI                               -> src/status/cli.c
        +-- 0x0002 File                              -> src/status/file.c
        +-- 0x0004 PE parser                         -> src/parsers/pe/status.c
        +-- 0x0005 COFF parser                       -> src/parsers/coff/status.c
        +-- 0x0006 Env parser                        -> src/parsers/env/status.c
        +-- 0x0007 PE loader                         -> src/loaders/pe/status.c
        +-- 0x0008 COFF loader                       -> src/loaders/coff/status.c
        +-- 0x0009 Syscall                           -> src/primitives/execution/syscalls/status.c
        +-- 0x000A Primitives                        -> src/primitives/status.c
```

---

## Status code facilities and domain headers

Status codes are separated by **Facility ID** (`snd_facility_id_t`) and constructed via `SND_MAKE_STATUS(facility_id, local_code)`. Each domain owns its status enum and string conversion lookup:

| Facility ID | Constant Name | Domain | Domain Header | Example Status Codes |
|---|---|---|---|---|
| `0x0000` | `SND_FACILITY_GENERIC` | Generic | `sindri/status/core.h` | `SND_SUCCESS`, `SND_STATUS_NULL_POINTER`, `SND_STATUS_ARCH_MISMATCH`, `SND_STATUS_TOO_MANY_ARGUMENTS` |
| `0x0001` | `SND_FACILITY_CLI` | CLI | `sindri/status/core.h` | `SND_STATUS_MISSING_COMMAND_LINE_ARGS`, `SND_STATUS_INVALID_COMMAND_LINE_ARG` |
| `0x0002` | `SND_FACILITY_FILE` | File I/O | `sindri/status/core.h` | `SND_STATUS_FILE_INVALID_PATH`, `SND_STATUS_FILE_CREATE_FAILED`, `SND_STATUS_FILE_READ_FAILED`, `SND_STATUS_FILE_TOO_LARGE` |
| `0x0003` | `SND_FACILITY_CONTEXT_MACHINES` | Loader/injection stages | `sindri/status/core.h` | `SND_STATUS_INVALID_STAGE`, `SND_STATUS_CORRUPTED_STAGE` |
| `0x0004` | `SND_FACILITY_PARSER_PE` | PE parsing | `sindri/parsers/pe/status.h` | `SND_STATUS_HEADER_DOS_SIGNATURE_INVALID`, `SND_STATUS_HEADER_NT_SIGNATURE_INVALID`, `SND_STATUS_HEADER_OFFSET_INVALID`, `SND_STATUS_EXPORT_SYMBOL_MISSING`, `SND_STATUS_DIRECTORY_ENTRY_MISSING`, `SND_STATUS_RELOCATION_BLOCK_INVALID` |
| `0x0005` | `SND_FACILITY_PARSER_COFF` | COFF parsing | `sindri/parsers/coff/status.h` | `SND_STATUS_HEADER_MACHINE_UNSUPPORTED`, `SND_STATUS_SYMBOL_ENTRY_MISSING`, `SND_STATUS_SYMBOL_NAKED_REJECTED`, `SND_STATUS_COFF_RELOCATION_TYPE_UNSUPPORTED` |
| `0x0006` | `SND_FACILITY_PARSER_ENV` | PEB/environment parsing | `sindri/parsers/env/status.h` | `SND_STATUS_MODULE_NOT_FOUND`, `SND_STATUS_NTDLL_CLEAN_NOT_INITIALIZED`, `SND_STATUS_PEB_GET_FAILED`, `SND_STATUS_MODULE_LIST_CORRUPTED` |
| `0x0007` | `SND_FACILITY_LOADER_PE` | Reflective PE loading | `sindri/loaders/pe/status.h` | `SND_STATUS_IMAGE_ENTRY_POINT_MISSING`, `SND_STATUS_DLL_INITIALIZATION_FAILED`, `SND_STATUS_LOCAL_EXECUTION_BLOCKED`, `SND_STATUS_RELOCATION_DIRECTORY_STRIPPED` |
| `0x0008` | `SND_FACILITY_LOADER_COFF` | COFF loading | `sindri/loaders/coff/status.h` | `SND_STATUS_COFF_LOADER_SYMBOL_MISSING`, `SND_STATUS_SYMBOL_ADDRESS_NULL`, `SND_STATUS_COFF_LOADER_MAP_SIZE_OVERFLOW`, `SND_STATUS_COFF_LOADER_RELOC_OUT_OF_RANGE` |
| `0x0009` | `SND_FACILITY_SYSCALL` | Syscalls | `sindri/primitives/syscalls.h` | `SND_STATUS_SSN_NOT_FOUND`, `SND_STATUS_GADGET_NOT_FOUND`, `SND_STATUS_SPOOF_GADGET_NOT_FOUND`, `SND_STATUS_RESOLVER_NOT_INITIALIZED`, `SND_STATUS_NTDLL_NOT_INITIALIZED` |
| `0x000A` | `SND_FACILITY_PRIMITIVES` | OS primitives | `sindri/primitives/status.h` | `SND_STATUS_ALLOC_FAILED`, `SND_STATUS_PROCESS_OPEN_FAILED`, `SND_STATUS_PROCESS_CREATE_FAILED`, `SND_STATUS_THREAD_QUEUE_FAILED`, `SND_STATUS_THREAD_RESUME_FAILED`, `SND_STATUS_SYSCALL_INVOKER_NOT_INITIALIZED` |

Domain status headers are pulled in by the umbrella `sindri.h` and by `src/status/status.c` for dispatch; `include/sindri/status.h` itself contains only `core.h` and `facility.h`. `snd_status_to_string()` extracts the facility via `SND_STATUS_FACILITY(status.code)` and dispatches lookup to domain-specific functions (`snd_pe_parser_status_to_string()`, `snd_ldr_pe_status_to_string()`, etc.). Injection currently reuses generic, primitive, and stage statuses rather than defining separate injection facilities.

---

## Conventions for domain code

1. **Return `snd_status_t`** from any function that can fail meaningfully.
2. **Propagate** upstream — do not swallow `os_error`; use `SND_ERR_W32` / `SND_ERR_NT` at the failure boundary.
3. **Use `SND_ERR_CTX`** for stage mismatches and resolver failures (stripped in SILENT).
4. **Do not** `printf` errors directly — use `SND_DEBUG_PRINT` or `snd_status_print`.
5. **Callers** test with `SND_FAILED(status)`, not `status.code != 0` alone (future-proof if success codes expand).

---

## Interaction with CRT-less builds

`SND_CRTLESS=ON` force-disables DEBUG and `SND_USE_PRINTF` because status context formatting requires `<stdio.h>` / `vsnprintf`.

CRT-less implants still receive `{ code, os_error }` from every API — only diagnostic strings disappear.

See [Building: CRT independence](../getting_started/building.md).

---

## Related documentation

- [Common infrastructure](../common/infrastructure.md)
- [State machines](state_machines.md) — stage sequence errors
- [Getting started](../getting_started/basic_usage.md) — status handling in PoCs

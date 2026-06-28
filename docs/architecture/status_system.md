# The Status System

SindriKit uses a universal **`snd_status_t`** return type instead of bare integers or NULL checks. Failures carry a framework error code and the underlying OS error (Win32 or NTSTATUS) in a single struct.

**Header:** `include/sindri/common/status.h`  
**Implementation:** `src/common/status.c`

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

## Status code ranges

Selected groupings from `snd_status_code_t`:

| Range | Domain | Examples |
|---|---|---|
| `0` | Success | `SND_SUCCESS` |
| Negative small ints | Generic | `SND_STATUS_INVALID_PARAMETER`, `SND_STATUS_BUFFER_TOO_SMALL` |
| `0x100` | CLI | `SND_STATUS_MISSING_COMMAND_LINE_ARGS` |
| `0x200` | File I/O | `SND_STATUS_FILE_READ_FAILED`, `SND_STATUS_ALLOC_FAILED` |
| `0x300` | PE parsing | `SND_STATUS_INVALID_NT_SIGNATURE`, `SND_STATUS_ARCH_MISMATCH` |
| `0x400` | Reflective load | `SND_STATUS_IMPORT_SYMBOL_RESOLVE_FAILED`, `SND_STATUS_INVALID_STAGE_SEQUENCE` |
| `0x500` | Syscalls | `SND_STATUS_SSN_NOT_FOUND` |
| `0x600` | PEB / env | `SND_STATUS_PEB_MODULE_NOT_FOUND` |
| `0x800` | OS / mapping | `SND_STATUS_VIRTUAL_ALLOC_FAILED`, `SND_STATUS_SECTION_MAP_FAILED` |
| `0x900` | Remote / access | `SND_STATUS_ACCESS_DENIED` |

Full enum: `include/sindri/common/status.h`.  
Per-domain references: [Common API](../common/api_reference.md#status-sindricommonstatush).

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

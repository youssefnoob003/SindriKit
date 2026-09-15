# Module Techniques

The modules subdomain defines how SindriKit loads DLLs and resolves exported functions in the **local** process. Reflective loaders and PE import fixups call `ctx->mod_api` rather than invoking `LoadLibraryA` or `GetProcAddress` directly.

## Paradigm 1: Win32 Resolution (`snd_mod_win`)

The `snd_mod_win` implementation uses standard Win32 APIs:

- **Load:** `LoadLibraryExA` with `LOAD_LIBRARY_SEARCH_DEFAULT_DIRS` (and `LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR` when available), falling back to `LoadLibraryA`
- **Export lookup:** `GetProcAddress`
- **Module base:** `GetModuleHandleW`

Only the three string-based callbacks are populated. Hash-based fields (`get_proc_address_hash`, `get_module_base_hash`) are `NULL` — callers must not invoke them on this backend.

### OpSec Implications

Every operation routes through monitored userland APIs and may leave plaintext DLL or function name strings in the binary. Suitable for diagnostics and standard profiles only.

## Paradigm 2: NT / PEB Resolution (`snd_mod_nt`)

The `snd_mod_nt` implementation avoids `kernel32.dll` import resolution APIs:

- **Load:** Checks the PEB module list first via `snd_peb_get_module_base`; if not loaded, resolves and calls `LdrLoadDll` from `ntdll.dll` via hash-based EAT parsing
- **Export lookup (string):** `snd_pe_get_export_address` with a PEB-walking module resolver callback
- **Export lookup (hash):** `snd_pe_get_export_address_hash`
- **Module base (string):** `snd_peb_get_module_base` (from `sindri/parsers/env/peb.h`)
- **Module base (hash):** `snd_peb_get_module_base_hash`

All five function pointer slots in `snd_module_api_t` are populated on this backend.

### OpSec Implications

Bypasses `GetProcAddress`, `GetModuleHandle`, and `LoadLibrary` telemetry. Hash-based resolution eliminates plaintext API and module name strings from the binary. Calls to `LdrLoadDll` and resolved NT exports still pass through in-process `ntdll.dll` stubs where inline hooks may exist.

This is the standard choice for stealth loader profiles (`snd_mod_nt` paired with `snd_mem_sys`).

## PEB Walking (`parsers/env`)

Low-level PEB traversal lives in the parsers domain, not in the modules primitive itself:

| Function | Header | Purpose |
|---|---|---|
| `snd_peb_get_module_base` | `sindri/parsers/env/peb.h` | Locate a loaded module by wide name |
| `snd_peb_get_module_base_hash` | `sindri/parsers/env/peb.h` | Locate a loaded module by compile-time hash |
| `snd_pe_get_export_address` | `sindri/parsers/pe/exports.h` | Resolve an export by name (forwarders use `snd_module_resolver_cb`) |
| `snd_pe_get_export_address_hash` | `sindri/parsers/pe/exports.h` | Resolve an export by compile-time hash (same forwarder resolver) |

The `snd_mod_nt` backend wires these parsers into the `snd_module_api_t` contract so higher-level domains (loaders, import fixups) remain agnostic to the resolution mechanism.

## No Syscall Backend

Module resolution does not have a `_sys` variant. Loading and EAT parsing are inherently userland operations; the evasion benefit comes from PEB walking and hash-based parsing rather than direct syscalls.

## Mixing Backends

A common pattern injects `snd_mod_nt` for import resolution while using `snd_mem_sys` for memory operations:

```c
snd_ldr_pe_ctx_t ctx = {0};
ctx.mem_api = &snd_mem_sys;
ctx.mod_api = &snd_mod_nt;
```

Import fixup (`snd_pe_resolve_imports`) requires a fully populated module API with working `load_library` and `get_proc_address` (or hash equivalents).

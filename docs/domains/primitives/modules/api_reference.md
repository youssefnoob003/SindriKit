# Modules: API Reference

This page documents the module and import resolution capabilities exported by the framework. Type definitions live in `include/sindri/primitives/os_api.h`; pre-built instances are declared in `include/sindri/primitives/modules.h`.

---

## Module API Instances (`sindri/primitives/modules.h`)

Two pre-built instances of the `snd_module_api_t` interface are exported globally. These pointers are typically assigned to `ctx.mod_api` during loader initialization.

| Symbol | Backend | Source |
|---|---|---|
| `snd_mod_win` | `LoadLibraryExA` / `GetProcAddress` / `GetModuleHandleW` | `src/primitives/modules/win.c` |
| `snd_mod_nt` | PEB walking + hash-based EAT resolution (`LdrLoadDll`, `snd_pe_get_export_address*`) | `src/primitives/modules/nt.c` |

> [!NOTE]
> Only `snd_mod_nt` populates the hash-based callbacks. On `snd_mod_win`, `get_proc_address_hash` and `get_module_base_hash` are `NULL`.

---

## The Interface (`sindri/primitives/os_api.h`)

### `snd_module_api_t`

Function pointer table defining the module and import resolution contract.

| Field | Signature | Description |
|---|---|---|
| `load_library` | `snd_status_t (*)(const char *module_name, HMODULE *out_module)` | Loads or resolves a module into the process |
| `get_proc_address` | `snd_status_t (*)(HMODULE hModule, const char *proc_name, FARPROC *out_proc)` | Resolves an exported symbol by name |
| `get_module_base` | `snd_status_t (*)(const wchar_t *module_name, PVOID *out_base)` | Retrieves the base address of an already-loaded module |
| `get_proc_address_hash` | `snd_status_t (*)(HMODULE hModule, DWORD proc_hash, FARPROC *out_proc)` | Hash-based export resolution |
| `get_module_base_hash` | `snd_status_t (*)(DWORD module_hash, PVOID *out_base)` | Hash-based module lookup via PEB walk |

#### `load_library`

| Parameter | Description |
|---|---|
| `module_name` | ASCII DLL name (e.g. `"kernel32.dll"`) |
| `out_module` | Receives the module handle / base address on success |

On `snd_mod_nt`, the backend first checks whether the module is already loaded via PEB walk before calling `LdrLoadDll`.

**Status codes:** `SND_STATUS_IMPORT_DLL_LOAD_FAILED`, `SND_STATUS_INVALID_PARAMETER`

#### `get_proc_address` / `get_proc_address_hash`

| Parameter | Description |
|---|---|
| `hModule` | Module base address |
| `proc_name` / `proc_hash` | Export name or compile-time hash (`sindri_hashes.h`) |
| `out_proc` | Receives the resolved function pointer |

**Status codes:** `SND_STATUS_IMPORT_SYMBOL_RESOLVE_FAILED`, `SND_STATUS_INVALID_PARAMETER`

#### `get_module_base` / `get_module_base_hash`

| Parameter | Description |
|---|---|
| `module_name` / `module_hash` | Wide module name or compile-time hash |
| `out_base` | Receives the module base address |

Does not load the module — returns a not-found status if the module is not in the PEB list.

**Status codes (`snd_mod_nt`):** `SND_STATUS_PEB_MODULE_NOT_FOUND`, `SND_STATUS_INVALID_PARAMETER`

**Status codes (`snd_mod_win` `get_module_base`):** `SND_STATUS_MODULE_NOT_FOUND`, `SND_STATUS_INVALID_PARAMETER`

---

## Related Parsers (`sindri/parsers/env/peb.h`)

PEB walking primitives used by `snd_mod_nt` are documented under the parsers domain:

- `snd_peb_get_module_base` — wide-name PEB walk
- `snd_peb_get_module_base_hash` — hash-based PEB walk
- `snd_peb_get_local` — inline accessor for the current process PEB

Export parsing (`snd_pe_get_export_address`, `snd_pe_get_export_address_hash`) lives in `sindri/parsers/pe/exports.h`. Both functions share the same forwarder resolver type (`snd_module_resolver_cb`); `snd_peb_get_module_base` is the typical wiring.

---

## Dependency Injection

Module backends are injected into loader contexts at initialization:

```c
snd_ldr_pe_ctx_t ctx = {0};
ctx.mod_api = &snd_mod_nt;  // or &snd_mod_win
```

The reflective loader and `snd_pe_resolve_imports` call `ctx.mod_api` throughout the import fixup pipeline. See [Dependency Injection](../../../architecture/dependency_injection.md).

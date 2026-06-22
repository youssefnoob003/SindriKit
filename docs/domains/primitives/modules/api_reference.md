# Modules: API Reference

This page documents the module capabilities exported by the framework.

---

## Module API Instances (`sindri/primitives/modules.h`)

Two pre-built instances of the `snd_module_api_t` interface are exported globally. These pointers are typically assigned to `ctx.mod_api` during loader or injector initialization.

| Symbol | Backend Paradigm |
|---|---|
| `snd_mod_win` | `LoadLibraryA` / `GetProcAddress` / `GetModuleHandleW` |
| `snd_mod_native` | PEB walking (`snd_peb_get_module_base_by_hash`) + hash-based export resolution (`snd_pe_get_export_address_by_hash`) |

---

## The Interface (`sindri/primitives/os_api.h`)

### `snd_module_api_t`

The function pointer table defining the module and import contract.

| Field | Signature | Description |
|---|---|---|
| `load_library` | `snd_status_t (*)(const char* module_name, PVOID* base)` | Loads or resolves a module into the process |
| `get_proc_address` | `snd_status_t (*)(PVOID base, const char* func_name, PVOID* func_addr)` | Resolves an exported symbol by name |
| `get_module_base` | `snd_status_t (*)(const wchar_t* module_name, PVOID* base)` | Retrieves the base address of a loaded module |



---

## PEB Walking (`sindri/primitives/peb.h`)

### `snd_peb_get_module_base`

Locates a loaded module's base address by walking `PEB->Ldr->InMemoryOrderModuleList` and comparing `BaseDllName` case-insensitively.

| Parameter | Type | Description |
|---|---|---|
| `module_name` | `const wchar_t*` | Case-insensitive target module name (e.g., `L"ntdll.dll"`) |
| `out_base` | `PVOID*` | Receives the base address of the located module |

**Returns:** `snd_status_t` — `SND_OK` on success.

---

### `snd_peb_get_module_base_by_hash`

Hash-based variant. Eliminates the plaintext module name string from the binary entirely.

| Parameter | Type | Description |
|---|---|---|
| `module_hash` | `DWORD` | Pre-computed hash of the target module name |
| `out_base` | `PVOID*` | Receives the base address of the located module |

**Returns:** `snd_status_t` — `SND_OK` on success.

---

## Global NTDLL State (`sindri/primitives/ntdll.h`)

SindriKit maintains global state for the `ntdll.dll` base address. This address is used ubiquitously by native components to extract clean stubs or parse export tables without relying on repetitive PEB lookups or the potentially hooked PEB-resident image.

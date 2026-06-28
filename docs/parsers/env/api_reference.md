# Env Parser: API Reference

Public env parser API under `include/sindri/parsers/env/`. Include `sindri/parsers/env.h` or `sindri/parsers.h` for the full surface.

NT structure definitions consumed by these functions are in `include/sindri/internal/nt/peb.h`.

---

## PEB Module Resolution (`sindri/parsers/env/peb.h`)

### `snd_peb_get_local`

Force-inline accessor for the current process PEB.

**Returns:** `PSND_PEB` — pointer to the local Process Environment Block

Supported architectures: x64, x86, ARM64. Unsupported architectures fail at compile time.

---

### `snd_peb_get_module_base`

Locates a loaded module by walking `InMemoryOrderModuleList` and comparing `BaseDllName`.

```c
snd_status_t WINAPI snd_peb_get_module_base(const wchar_t *module_name, PVOID *out_base);
```

| Parameter | Description |
|---|---|
| `module_name` | Case-insensitive short name (e.g. `L"ntdll.dll"`) |
| `out_base` | Receives `DllBase` on success |

**Returns:** `SND_OK`, `SND_STATUS_INVALID_PARAMETER`, `SND_STATUS_PEB_MODULE_NOT_FOUND`

**Source:** `src/parsers/env/peb.c`

---

### `snd_peb_get_module_base_hash`

Hash-based module lookup — no wide-string comparison at runtime.

```c
snd_status_t WINAPI snd_peb_get_module_base_hash(DWORD module_hash, PVOID *out_base);
```

| Parameter | Description |
|---|---|
| `module_hash` | Compile-time hash from `sindri_hashes.h` (e.g. `SND_HASH_NTDLL_DLL`) |
| `out_base` | Receives `DllBase` on success |

**Returns:** `SND_OK`, `SND_STATUS_INVALID_PARAMETER`, `SND_STATUS_PEB_MODULE_NOT_FOUND`

---

## NT Structures (`sindri/internal/nt/peb.h`)

These layouts are not public parser API but are referenced by env functions:

| Type | Description |
|---|---|
| `SND_PEB` | Process Environment Block |
| `SND_PEB_LDR_DATA` | Loader data with three module lists |
| `SND_LDR_DATA_TABLE_ENTRY` | Per-module entry (`DllBase`, `BaseDllName`, `FullDllName`, …) |
| `SND_RTL_USER_PROCESS_PARAMETERS` | Process parameters (command line, paths, environment) |
| `SND_UNICODE_STRING` | NT counted string |

---

## Usage as Export Forwarder Resolver

Both export functions accept `snd_module_resolver_cb`:

```c
typedef snd_status_t (WINAPI *snd_module_resolver_cb)(const wchar_t *module_name, PVOID *out_base);
```

`snd_peb_get_module_base` matches this signature and is the standard resolver for export forwarders:

```c
FARPROC func = NULL;
snd_status_t status = snd_pe_get_export_address_hash(
    ntdll_base,
    SND_SYS_DLL_SIZE_DEFAULT,
    SND_HASH_NTOPENSECTION,
    &func,
    snd_peb_get_module_base
);
```

The hash-based export function uses the same wide-name resolver — not `snd_module_resolver_hash_cb`.

---

## Status Codes

| Code | Meaning |
|---|---|
| `SND_STATUS_PEB_MODULE_NOT_FOUND` | Module not present in the PEB loader list |
| `SND_STATUS_PEB_PROCESS_PARAMETERS_NOT_FOUND` | Reserved for future process-parameter helpers |
| `SND_STATUS_INVALID_PARAMETER` | NULL output pointer or zero hash |

---

## Related Documentation

- [PE export resolution](../pe/api_reference.md) — `snd_pe_get_export_address`, forwarders
- [Module primitives](../../domains/primitives/modules/api_reference.md) — `snd_mod_nt` wiring
- [Dependency Injection](../../architecture/dependency_injection.md) — resolver callback types in `os_api.h`

# Mapping: API Reference

This page documents the section mapping capabilities exported by the framework. All type definitions live in `include/sindri/primitives/os_api.h`; pre-built instances are declared in `include/sindri/primitives/mapping.h`.

---

## Mapping API Instances (`sindri/primitives/mapping.h`)

Three pre-built instances of the `snd_mapping_api_t` interface are exported globally.

| Symbol | Backend | Source |
|---|---|---|
| `snd_map_win` | Win32 API (`OpenFileMappingW`, `MapViewOfFile`, `CloseHandle`) | `src/primitives/mapping/win.c` |
| `snd_map_nt` | NT API via PEB + EAT resolution (`NtOpenSection`, `NtMapViewOfSection`, `NtClose`) | `src/primitives/mapping/nt.c` |
| `snd_map_sys` | Direct syscalls via SSN resolution + ASM stub | `src/primitives/mapping/sys.c` |

---

## The Interface (`sindri/primitives/os_api.h`)

### `snd_mapping_api_t`

Function pointer table defining the section mapping contract.

| Field | Signature | Description |
|---|---|---|
| `open` | `snd_status_t (*)(const wchar_t *section_name, HANDLE *out_handle)` | Opens a named section object with read access |
| `view` | `snd_status_t (*)(HANDLE section_handle, PVOID *out_base, SIZE_T *out_size)` | Maps a read-only view into the current process |
| `close` | `snd_status_t (*)(HANDLE handle)` | Closes the section handle |

#### `open`

| Parameter | Description |
|---|---|
| `section_name` | Section object name. NT backends accept absolute Object Manager paths (e.g. `\KnownDlls\ntdll.dll`). The Win32 backend accepts DOS or Win32 namespace paths only. |
| `out_handle` | Receives the opened section handle on success |

**Status codes:** `SND_STATUS_SECTION_OPEN_FAILED` (with OS or NTSTATUS detail), `SND_STATUS_INVALID_PARAMETER`

#### `view`

| Parameter | Description |
|---|---|
| `section_handle` | Handle returned by `open` |
| `out_base` | Receives the base address of the mapped view |
| `out_size` | Receives the size of the mapped region |

All backends map with read-only protection (`PAGE_READONLY` / `FILE_MAP_READ`). The Win32 backend derives `out_size` from `VirtualQuery`; NT and syscall backends use the size returned by `NtMapViewOfSection`.

**Status codes:** `SND_STATUS_SECTION_MAP_FAILED`, `SND_STATUS_INVALID_PARAMETER`

#### `close`

Closes the section handle. The mapped view remains valid after the handle is closed (standard Windows section semantics).

**Status codes:** `SND_STATUS_HANDLE_CLOSE_FAILED`, `SND_STATUS_INVALID_PARAMETER`

---

## Object Manager Helper (`sindri/primitives/object_manager.h`)

### `snd_om_knowndll_map`

Maps a DLL from the `\KnownDlls` (x64) or `\KnownDlls32` (x86) Object Manager directory into the current process.

```c
snd_status_t snd_om_knowndll_map(
    const snd_mapping_api_t *config,
    const wchar_t *dll_name,
    PVOID *out_base_address
);
```

| Parameter | Description |
|---|---|
| `config` | Mapping backend to use (`&snd_map_nt` or `&snd_map_sys` for KnownDlls paths) |
| `dll_name` | DLL file name only (e.g. `L"ntdll.dll"`) — the helper prepends `SND_TARGET_KNOWNDLLS_DIR` |
| `out_base_address` | Receives the base address of the mapped view |

**Status codes:** `SND_STATUS_INVALID_PARAMETER`, `SND_STATUS_NOT_INITIALIZED` (missing function pointers in `config`), `SND_STATUS_SECTION_OPEN_FAILED`, `SND_STATUS_SECTION_MAP_FAILED`

**Source:** `src/primitives/object_manager/knowndlls.c`

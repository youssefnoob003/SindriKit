# Env Parser: Techniques

The env subdomain provides runtime introspection of Windows process environment structures without calling Win32 APIs. Today this means PEB access and module list walking; the subdomain is the designated home for future TEB and process-parameter helpers.

---

## Why a Separate Subdomain

PE parsing interprets **file-format structures** inside a buffer (headers, directories, sections). Env parsing interprets **OS-maintained process state** in live memory (PEB, loader lists, thread blocks).

Keeping these separate reflects the code layout:

```
include/sindri/parsers/
├── pe/          ← buffer-backed PE format parsing
└── env/         ← live process environment structures
```

NT layout definitions (`SND_PEB`, `SND_LDR_DATA_TABLE_ENTRY`, etc.) live in `include/sindri/internal/nt/peb.h`. Env parsers consume these layouts; they do not duplicate Windows SDK headers to avoid pulling in monitored imports.

---

## The Process Environment Block (PEB)

The PEB is a user-mode structure describing process-wide state. SindriKit's simplified layout (`SND_PEB`) exposes the fields relevant to module enumeration and future process-parameter access:

| Field | Purpose |
|---|---|
| `BeingDebugged` | Anti-debug checks (future env helpers) |
| `Ldr` | Pointer to loader data (`SND_PEB_LDR_DATA`) |
| `ProcessParameters` | Command line, image path, environment (future helpers) |

### Accessing the Local PEB

`snd_peb_get_local()` reads the current thread's PEB pointer from the CPU segment register:

| Architecture | Mechanism | Offset |
|---|---|---|
| x64 (Intel/AMD) | `__readgsqword` | `0x60` |
| x86 | `__readfsdword` | `0x30` |
| ARM64 | `__readx18qword` | `0x60` |

This is a force-inline in the header — no function call, no import table entry.

---

## Module List Walking

Loaded modules are tracked in three linked lists inside `SND_PEB_LDR_DATA`:

```
InLoadOrderModuleList
InMemoryOrderModuleList      ← used by SindriKit
InInitializationOrderModuleList
```

SindriKit walks **`InMemoryOrderModuleList`**, which matches the order used by most manual mapping and evasion tooling.

Each list entry is an `SND_LDR_DATA_TABLE_ENTRY` embedded in a `LIST_ENTRY`. The walker recovers the structure via the standard `CONTAINING_RECORD` offset:

```c
PSND_LDR_DATA_TABLE_ENTRY entry =
    (PSND_LDR_DATA_TABLE_ENTRY)((BYTE *)list_entry - offsetof(SND_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks));
```

### Name Matching

Module lookup compares against `BaseDllName` (short name, e.g. `ntdll.dll`), not `FullDllName` (full path). Comparisons are case-insensitive.

**String mode** (`snd_peb_get_module_base`): exact length match + `snd_wcsnicmp`.

**Hash mode** (`snd_peb_get_module_base_hash`): copies `BaseDllName` to a stack buffer, lowercases via `snd_hash_wide_lower`, compares against the compile-time module hash from `sindri_hashes.h`.

---

## Integration with PE Export Resolution

NT and syscall primitives combine env and PE parsers in a two-step pattern:

1. **Locate `ntdll.dll`** via `snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll)`
2. **Resolve exports** via `snd_pe_get_export_address_hash(..., snd_peb_get_module_base)`

The export resolver's forwarder callback is `snd_module_resolver_cb` — the same wide-name signature as `get_module_base` on `snd_module_api_t`. Passing `snd_peb_get_module_base` wires PEB walking into export forwarder resolution without a separate hash resolver.

KnownDlls bootstrapping bypasses the PEB for the *initial* clean `ntdll` copy but subsequent forwarders still hit the PEB-resident module list.

---

## Integration with Module Primitives

`snd_mod_nt` wires env parsers directly into the module API table:

| `snd_module_api_t` field | Implementation |
|---|---|
| `get_module_base` | `snd_peb_get_module_base` |
| `get_module_base_hash` | `snd_peb_get_module_base_hash` |

This is why module resolution docs reference `parsers/env/peb.h` rather than a removed `primitives/peb` module.

---

## Safety Considerations

- **No locking:** The loader lists can change if another thread loads a DLL. SindriKit does not acquire the loader lock; callers should avoid concurrent loads during walks.
- **Null checks:** Both public functions validate `out_base` and return `SND_STATUS_INVALID_PARAMETER` on bad input. A missing module returns `SND_STATUS_PEB_MODULE_NOT_FOUND`.
- **Hash collisions:** Hash-based lookup assumes compile-time hashes from the project's hash manifest are collision-free for the target module set.

---

## Future: TEB and Process Parameters

The env subdomain will expand to cover:

- **TEB** — thread-local storage, stack limits, thread ID (via segment register reads analogous to PEB)
- **Process parameters** — command line, image path, current directory, environment block via `PEB->ProcessParameters`

These will follow the same pattern: NT layouts in `internal/nt/`, public accessors in `parsers/env/`.

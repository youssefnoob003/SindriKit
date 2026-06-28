# Hash Manifest (`config/hashes.ini`)

**Location:** `config/hashes.ini`

This INI-style manifest is the single source of truth for compile-time API string hashes. CMake runs `scripts/generate_hashes.py` at configure time, emitting **`sindri_hashes.h`** into the build directory (`${CMAKE_BINARY_DIR}/generated/`).

## Purpose

Every time SindriKit needs to resolve a Windows API — whether walking the PEB for a module base, parsing an export table for a function address, or matching an import descriptor's DLL name — it compares a runtime-computed hash against a compile-time constant. This manifest defines exactly which strings get pre-hashed.

## Format Specification

### Module Sections: `[module::<dll_name>]`

Groups API names under a specific DLL. The build system generates a hash for both the module name itself and every API listed beneath it.

```ini
[module::ntdll.dll]
NtAllocateVirtualMemory
NtProtectVirtualMemory
NtFreeVirtualMemory
NtOpenSection
NtMapViewOfSection
NtClose
LdrLoadDll
```

This produces:
- `SND_HASH_NTDLL_DLL` — hash of the module name `ntdll.dll`
- `SND_HASH_NTALLOCATEVIRTUALMEMORY` — hash of the export name
- ... one per API listed.

### Extras Section: `[extras]` (optional)

Standalone strings that do not belong to any module (e.g., registry keys, object paths). The generator supports this section (`scripts/generate_hashes.py`), but **`config/hashes.ini` does not currently define one** — only `[module::…]` sections are present today.

When added, each line produces an individual `SND_HASH_…` define (no module hash for the section):

```ini
[extras]
SomeStandaloneString
```

### Comments and Whitespace

Lines beginning with `#` are comments and are ignored by the parser. Blank lines are ignored.

## Adding a New Domain

When implementing a new SindriKit domain that requires runtime API resolution:

1. Add a new `[module::<dll_name>]` section for the target DLL.
2. List every API the domain implementation calls beneath it.
3. Rebuild. One `cmake --build` regenerates the entire header automatically.

```ini
# -- user32.dll ---------------------------------------------------------------
# APIs required by a future GUI evasion domain.
[module::user32.dll]
MessageBoxA
FindWindowA
```

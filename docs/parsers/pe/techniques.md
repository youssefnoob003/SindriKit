# PE Parser: Techniques

The PE parser is the structural foundation every higher-level SindriKit domain sits on. Reflective loaders use it to validate and map payloads. Syscall and NT primitives use export resolution to locate `Nt*` stubs. Import fixups use it to walk descriptor tables and patch the IAT.

**Canonical reference:** [PE Format Specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)

---

## PE Format Overview

A Portable Executable file is laid out as a sequence of structured headers followed by variable-length sections:

```
[ IMAGE_DOS_HEADER         ]  "MZ" — compatibility stub and e_lfanew offset
[ MS-DOS Stub              ]  legacy stub program
[ IMAGE_NT_HEADERS         ]  "PE\0\0" signature + FILE_HEADER + OPTIONAL_HEADER
[ IMAGE_SECTION_HEADER[]   ]  one per section: name, VirtualAddress, PointerToRawData, sizes
[ Section Data             ]  .text, .data, .rdata, .reloc, .tls, ...
```

The Optional Header contains the `DataDirectory` array — 16 entries pointing to the Import Table, Export Table, Base Relocation Table, TLS Table, and others. All in-image references use Relative Virtual Addresses (RVAs): offsets from the image base, not file offsets.

---

## Zero-Copy Parsing Model

`snd_pe_parse` performs **zero-copy parsing**. Pointers inside `snd_pe_parser_t` (`dos`, `nt`, `section_head`) point directly into the caller's buffer. No heap allocations occur inside the parser.

The parser context lifecycle is strictly bound to the backing `snd_buffer_t`. If the buffer is freed or relocated, all cached pointers become invalid.

---

## The `snd_pe_parser_t` Context

`snd_pe_parse` populates a context that caches critical pointers and flags for subsequent directory operations:

| Field | Description |
|---|---|
| `source` | Copy of the input buffer descriptor (base + size) |
| `dos` | Pointer to `IMAGE_DOS_HEADER` inside `source` |
| `nt.nt32` / `nt.nt64` | Union covering PE32 and PE32+ NT headers |
| `section_head` | Pointer to the first `IMAGE_SECTION_HEADER` |
| `string_table` | COFF string table pointer (raw files only; `NULL` for mapped images) |
| `is_64bit` | `TRUE` for PE32+ |
| `is_dll` | `TRUE` if `IMAGE_FILE_DLL` is set |
| `is_mapped` | Layout mode flag (see below) |
| `sections_count` | Number of section headers (clamped to available buffer space) |
| `imports_rva` / `import_size` | Cached import directory location |

Use `SND_PE_GET_NT_FIELD(parser, field)` to read Optional Header fields without branching on bitness at every call site.

---

## The `is_mapped` Flag: File vs. Memory Layout

This is the most operationally significant choice at parse time.

**Raw file layout (`is_mapped = FALSE`):** Sections live at `PointerToRawData` in the file. RVA translation requires the section table — finding which section contains the RVA and computing the file offset as `PointerToRawData + (rva - VirtualAddress)`.

**Mapped image layout (`is_mapped = TRUE`):** Sections are laid out at their `VirtualAddress` from the image base. An RVA translates directly to `base + rva`.

`snd_pe_rva_to_ptr` branches on `parser->is_mapped`. Typical usage:

| Buffer | `is_mapped` | Consumer |
|---|---|---|
| Payload read from disk | `FALSE` | Reflective loader input validation |
| KnownDlls-mapped `ntdll.dll` | `TRUE` | Syscall SSN resolution, export lookup |
| Locally mapped reflective image | `TRUE` | Import fixup, relocations, TLS |

Passing the wrong flag produces out-of-bounds reads (caught by bounds checks) or silent reads from wrong offsets.

> [!NOTE]
> `snd_pe_resolve_imports`, `snd_pe_apply_relocations`, and `snd_pe_execute_tls_callbacks` **require** `is_mapped = TRUE`. They operate on a virtually laid-out image, not a raw file buffer.

---

## Bounds Checking Philosophy

Every directory and section access routes through `snd_pe_rva_to_ptr`, which validates that `(rva, size)` lies entirely within the tracked buffer before returning a pointer. Violations return `NULL`; callers must check.

Header parsing additionally uses `snd_buffer_bounds_check` from the common buffer layer during the bootstrap parse in `snd_pe_parse`.

### The 4KB Bootstrap Window (`SND_SYS_DLL_SIZE_DEFAULT`)

Parsing in-memory system DLLs presents a bootstrapping problem: `OptionalHeader.SizeOfImage` lives inside the headers, but reading it requires a known buffer bound first.

`SND_SYS_DLL_SIZE_DEFAULT` (`0x1000`) is a conservative one-page bootstrap window — sufficient for DOS + NT headers + section table on known system DLLs.

The export resolver (`snd_pe_get_export_address*`) automatically expands the parser's tracked size to `SizeOfImage` when the caller passes `SND_SYS_DLL_SIZE_DEFAULT` and the parsed image reports a larger size.

> [!IMPORTANT]
> When using `snd_pe_parse` directly on an in-memory image, update `parser->source.size` to `OptionalHeader.SizeOfImage` after a successful parse before calling directory functions. Leaving it at `0x1000` causes subsequent `snd_pe_rva_to_ptr` calls for imports, exports, and relocations to fail bounds checks.

---

## Export Table Resolution

Export resolution is implemented by a **single unified engine** (`pe_get_export_address_impl` in `src/parsers/pe/exports.c`) shared by both public entry points:

| Function | Lookup mode |
|---|---|
| `snd_pe_get_export_address` | Name string, or ordinal via `MAKEINTRESOURCE` |
| `snd_pe_get_export_address_hash` | Compile-time hash from `sindri_hashes.h` (algorithm set by `SND_HASH_ALGO`) |

Both functions accept the same forwarder resolver type: `snd_module_resolver_cb` — a wide-name module base lookup (`const wchar_t *module_name, PVOID *out_base`). There is no separate hash-based resolver callback for forwarders; the engine converts forwarder DLL prefixes to wide strings and calls the same resolver.

### Resolution Flow

1. Bootstrap-parse the target image with `is_mapped = TRUE`.
2. Expand buffer size from `SizeOfImage` when using `SND_SYS_DLL_SIZE_DEFAULT`.
3. Locate the export directory via `snd_pe_get_directory`.
4. Resolve by ordinal (`IS_INTRESOURCE`), name iteration, or hash comparison.
5. If the function RVA falls inside the export directory itself, treat it as a **forwarder**.

### Export Forwarders

Forwarder RVAs point into the export directory and contain strings like `NTDLL.RtlMoveMemory`. The engine:

1. Splits on `.`, appends `.dll` to the module prefix.
2. Calls `resolver(wfwd_dll, &fwd_base)` to locate the target module.
3. Recurses into the forwarded export (name or re-hashed name for hash mode).
4. Supports ordinal forwarders (`MODULE.#123`).

Recursion depth is capped at `SND_FWD_MAX_DEPTH` (4). Pass `NULL` as `resolver` to fail forwarders with `SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED`.

Typical resolver wiring in NT primitives:

```c
status = snd_pe_get_export_address_hash(
    ntdll, SND_SYS_DLL_SIZE_DEFAULT, SND_HASH_NTOPENSECTION,
    &func_addr, snd_peb_get_module_base  /* wide-name PEB walk */
);
```

---

## Import Table Parsing

The import table is a null-terminated array of `IMAGE_IMPORT_DESCRIPTOR` structures. Each descriptor references:

- **Import Name Table (INT)** — original thunks (`OriginalFirstThunk`, or `FirstThunk` if absent)
- **Import Address Table (IAT)** — patched at load time (`FirstThunk`)

`snd_pe_resolve_imports` walks each descriptor:

1. Read the DLL name and call `mod_api->load_library`.
2. Iterate the thunk array. Ordinals are detected via `SND_PE_SNAP_BY_ORDINAL32`/`64` (high bit set).
3. Resolve symbols via `mod_api->get_proc_address` (including ordinal imports via `MAKEINTRESOURCE`).
4. Write resolved addresses into IAT slots.

All module loading and symbol resolution is delegated to the injected `mod_api`, inheriting its OpSec profile (`snd_mod_win` vs `snd_mod_nt`).

**Requirements:** mapped image (`is_mapped = TRUE`), non-NULL `mod_api->load_library` and `mod_api->get_proc_address`.

---

## Base Relocations

PE images embed absolute virtual addresses assuming load at `ImageBase`. When mapped elsewhere, every pointer is wrong by `delta = actual_base - preferred_base`.

The `.reloc` section contains `IMAGE_BASE_RELOCATION` blocks. Each block covers a 4KB page with 12-bit offsets and 4-bit type tags. SindriKit patches:

- `IMAGE_REL_BASED_DIR64` — QWORD add on x64
- `IMAGE_REL_BASED_HIGHLOW` — DWORD add on x86
- `IMAGE_REL_BASED_ABSOLUTE` — skipped (padding)

If `delta_offset == 0`, the function returns immediately with `SND_OK`.

If the image has no relocation directory, an empty directory, or `IMAGE_FILE_RELOCS_STRIPPED` is set while `delta != 0`, the function returns `SND_STATUS_RELOCATION_DIRECTORY_INVALID`.

> [!NOTE]
> The mapped image must have at least `PAGE_READWRITE` on affected pages — the function physically overwrites pointer-sized values in `.text` and `.data`.

---

## TLS Callbacks

TLS callback arrays are located via the TLS data directory. `snd_pe_get_tls_callbacks` translates `AddressOfCallBacks` from image-base-relative to RVA, accounting for mapped vs preferred base.

`snd_pe_execute_tls_callbacks` iterates the null-terminated callback pointer array and invokes each entry with the specified reason (`DLL_PROCESS_ATTACH`, etc.). Requires a mapped image and matching host/payload architecture.

---

## Internal Section Helpers

`section_utils.h` exposes helpers used by the reflective loader engine (`snd_pe_section_name`, `snd_pe_section_copy_size`, `snd_pe_section_loaded_size`). These are not part of the general-purpose parser contract and are marked internal in their header comments.

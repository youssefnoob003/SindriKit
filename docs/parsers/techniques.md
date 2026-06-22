# PE Parser: Techniques

The parsers subsystem is the structural foundation that every higher-level SindriKit domain sits on. The reflective loader uses it to validate and map payloads. The syscall resolution pipeline uses it to walk `ntdll.dll`'s export table. The import resolver uses it to iterate descriptor tables and patch the IAT. Understanding how the parser works — and where it makes safety trade-offs — is necessary for using it correctly in both standard and adversarial contexts.

**Canonical reference:** [PE Format Specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format) — Microsoft Learn

---

## PE Format Overview

A Portable Executable file is laid out as a sequence of structured headers followed by a set of variable-length sections:

```
[ IMAGE_DOS_HEADER         ]  "MZ" — compatibility stub and e_lfanew offset
[ MS-DOS Stub              ]  legacy stub program
[ IMAGE_NT_HEADERS         ]  "PE\0\0" signature + FILE_HEADER + OPTIONAL_HEADER
[ IMAGE_SECTION_HEADER[]   ]  one per section: name, VirtualAddress, PointerToRawData, sizes
[ Section Data             ]  .text, .data, .rdata, .reloc, .tls, ...
```

The Optional Header (despite its name, mandatory for executable images) contains the `DataDirectory` array — 16 entries pointing to structures like the Import Table, Export Table, Base Relocation Table, and TLS Table. All directory and section references within a PE are expressed as Relative Virtual Addresses (RVAs): offsets from the image base, not file offsets.

---

## The `snd_pe_parser_t` Context

`snd_pe_parse` populates a `snd_pe_parser_t` structure that caches all critical pointers and flags needed for subsequent directory operations. The key fields:

| Field | Type | Description |
|---|---|---|
| `source` | `snd_buffer_t` | Copy of the buffer descriptor (base pointer + size) |
| `dos` | `PIMAGE_DOS_HEADER` | Direct pointer into the source buffer |
| `nt.nt32` / `nt.nt64` | union | NT headers pointer — union covers both PE32 and PE32+ |
| `section_head` | `PIMAGE_SECTION_HEADER` | Pointer to the first section header |
| `is_64bit` | `BOOL` | TRUE for PE32+ (64-bit), FALSE for PE32 (32-bit) |
| `is_dll` | `BOOL` | TRUE if `IMAGE_FILE_DLL` characteristic is set |
| `is_mapped` | `BOOL` | TRUE if the buffer is a loaded image, FALSE if it is a raw file |
| `sections_count` | `DWORD` | Number of section headers |
| `imports_rva` | `DWORD` | Cached RVA of the import directory |
| `import_size` | `DWORD` | Cached size of the import directory |

The `SND_PE_GET_NT_FIELD(parser, field)` macro abstracts the 32/64-bit union, allowing callers to read header fields without branching on `is_64bit` at every call site.

---

## The `is_mapped` Flag: File vs. Memory Layout

This flag is the most operationally significant choice made at parse time, and getting it wrong produces silent, hard-to-diagnose failures.

**Raw file layout (disk):** Sections live at `PointerToRawData` within the file. A section whose `VirtualAddress` is `0x1000` might have a `PointerToRawData` of `0x400`. A tool reading a raw PE from disk must convert RVAs using the section table — finding which section the RVA falls into and computing the file offset as `PointerToRawData + (rva - VirtualAddress)`.

**Mapped image layout (memory):** After a PE is mapped into virtual memory, sections live at their `VirtualAddress` from the image base. The OS (or the reflective loader) has already performed the section layout. An RVA translates directly to `base + rva`.

`snd_pe_rva_to_ptr` checks `parser->is_mapped` and branches accordingly. The same parser code path is used for:
- The reflective loader's **input payload** — a raw file read from disk → `is_mapped = FALSE`
- A **KnownDlls-mapped `ntdll.dll`** used for SSN resolution → `is_mapped = TRUE`

Passing the wrong flag produces either out-of-bounds reads (crashing cleanly if bounds checks fire) or silent reads from wrong offsets producing garbage data.

---

## Bounds Checking and Parser Safety

SindriKit's parser is designed to fail safely against malformed or weaponized PE inputs. Every data directory and section access is routed through `snd_pe_rva_to_ptr`, which validates that the requested `(rva, size)` region lies entirely within the tracked buffer bounds before returning a pointer. On any violation, it returns `NULL`, which callers must check.

### The 4KB Bootstrap Window (`SND_SYS_DLL_SIZE_DEFAULT`)

Parsing in-memory system DLLs (e.g., a KnownDlls-mapped `ntdll.dll`) presents a bootstrapping problem: the total image size is stored inside the `OptionalHeader.SizeOfImage` field, but to read that field, the headers must first be parsed, which requires a known buffer bound.

`SND_SYS_DLL_SIZE_DEFAULT` is defined as `0x1000` (4096 bytes — one virtual page). It acts as a conservative initial bound: sufficient to contain the DOS header, NT headers, and section table for every known system DLL, but small enough to limit exposure if the input is malformed.

> [!IMPORTANT]
> After `snd_pe_parse` succeeds on an in-memory image, the operator **must** update the parser's `source.size` field to `OptionalHeader.SizeOfImage` before calling any directory resolution function. Leaving it clamped at `0x1000` will cause all subsequent `snd_pe_rva_to_ptr` calls for data directories (imports, exports, relocations) to be rejected as out-of-bounds.

---

## Export Table Parsing

SindriKit resolves exports from a PE by walking the `IMAGE_EXPORT_DIRECTORY`. The directory contains three parallel arrays:
- `AddressOfNames` — RVAs to null-terminated function name strings
- `AddressOfNameOrdinals` — 16-bit ordinal indices into the functions array
- `AddressOfFunctions` — RVAs to the actual function entry points

For name-based resolution, SindriKit iterates the names array (or uses binary search when the names are sorted, as is standard) to find the target, retrieves the corresponding ordinal, then indexes `AddressOfFunctions`.

For **hash-based resolution** (`snd_pe_get_export_address_by_hash`), each export name is hashed at resolution time using the configured algorithm (DJB2 by default) and compared against the target hash. This eliminates all plaintext function name strings from the binary.

### Export Forwarders

Some exports are forwarders — their RVA points into the export directory itself (rather than a function body) and contains a string like `NTDLL.RtlMoveMemory`. SindriKit resolves forwarders recursively by splitting the string, looking up the named module via the injected `resolver` callback, and repeating the export search in the target module. The recursion depth is capped at `SND_FWD_MAX_DEPTH` (4) to prevent cycles or intentionally deep forwarder chains from looping indefinitely.

Pass `NULL` as the `resolver` argument to forcefully fail forwarder resolution with `SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED`.

---

## Import Table Parsing

The import table is a null-terminated array of `IMAGE_IMPORT_DESCRIPTOR` structures, one per dependency DLL. Each descriptor points to two parallel thunk arrays:
- **Import Name Table (INT)** — original references, untouched after loading
- **Import Address Table (IAT)** — populated with resolved addresses at load time

For each descriptor, `snd_pe_resolve_imports`:
1. Reads the DLL name from the descriptor and calls `mod_api->load_library`.
2. Iterates the thunk array. Each thunk is either ordinal-based (detected with `SND_PE_SNAP_BY_ORDINAL32`/`64` macros — high bit set) or name-based (`IMAGE_IMPORT_BY_NAME` entry).
3. Calls `mod_api->get_proc_address` to resolve the symbol.
4. Writes the resolved address directly into the IAT slot.

Because all module and symbol resolution is routed through the injected `mod_api`, the import resolution step inherits the full OpSec profile of the configured module primitive — Win32-visible or PEB-walking native.

---

## Base Relocations

PE images embed absolute virtual addresses in their code and data sections (pointer fields, vtable entries, etc.) at compile time, assuming the image will be loaded at its preferred `ImageBase`. When a reflective loader allocates memory at a different address, all embedded pointers are wrong by a fixed constant: `delta = actual_base - preferred_base`.

The `.reloc` section contains a sequence of `IMAGE_BASE_RELOCATION` blocks. Each block covers a 4KB page of the image and contains a list of 12-bit offsets (plus a 4-bit type tag). For each entry of type `IMAGE_REL_BASED_DIR64` (x64) or `IMAGE_REL_BASED_HIGHLOW` (x86), SindriKit adds `delta` to the QWORD/DWORD at `virtual_base + block.VirtualAddress + entry.Offset`. Each write is bounds-checked before application.

If the image happens to land at its preferred base, `delta` is zero and the entire pass is a no-op with no writes.

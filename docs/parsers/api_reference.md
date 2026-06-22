# PE Parser: API Reference

This page documents the full public API surface of the parsers subsystem. Headers live under `include/sindri/parsers/pe/`.

---

## Core Parser (`sindri/parsers/pe/pe_parser.h`)

### `snd_pe_parse`

Validates and parses a raw or mapped PE buffer into a `snd_pe_parser_t` context. Checks the DOS signature (`MZ`), the NT signature (`PE\0\0`), and the Optional Header magic (`0x10B` for PE32, `0x20B` for PE32+). Populates all fields of the output structure on success.

| Parameter | Type | Description |
|---|---|---|
| `buf` | `const snd_buffer_t*` | Buffer descriptor containing the PE data (pointer + size) |
| `is_mapped` | `BOOL` | `FALSE` for a raw file, `TRUE` for a memory-mapped image |
| `parser` | `snd_pe_parser_t*` | Output context to populate |

**Returns:** `snd_status_t` — `SND_OK` on success, or a parse error (`SND_STATUS_INVALID_DOS_SIGNATURE`, `SND_STATUS_INVALID_NT_SIGNATURE`, `SND_STATUS_UNSUPPORTED_OPTIONAL_HEADER_MAGIC`, etc.).

---

### `SND_PE_GET_NT_FIELD(parser, field)`

Macro for reading a field from the NT Optional Header without branching on `is_64bit` at every call site. Selects `nt.nt64->field` or `nt.nt32->field` based on `parser->is_64bit`.

```c
DWORD size_of_image = SND_PE_GET_NT_FIELD(&ctx.pe, OptionalHeader.SizeOfImage);
```

---

### `SND_SYS_DLL_SIZE_DEFAULT`

**Value:** `0x1000` (4096 bytes)

Bootstrap bound used when parsing the headers of an in-memory system DLL before `SizeOfImage` is known. After `snd_pe_parse` succeeds, the caller must update `parser->source.size` to `SizeOfImage` before calling any directory resolution function.

---

### `snd_pe_parser_t`

| Field | Type | Description |
|---|---|---|
| `source` | `snd_buffer_t` | Buffer descriptor (base pointer + tracked size) |
| `dos` | `PIMAGE_DOS_HEADER` | Pointer to the DOS header |
| `nt.nt32` | `PIMAGE_NT_HEADERS32` | NT headers pointer for 32-bit images (valid when `is_64bit == FALSE`) |
| `nt.nt64` | `PIMAGE_NT_HEADERS64` | NT headers pointer for 64-bit images (valid when `is_64bit == TRUE`) |
| `section_head` | `PIMAGE_SECTION_HEADER` | Pointer to the first section header entry |
| `string_table` | `BYTE*` | COFF string table pointer (used for long section names) |
| `is_64bit` | `BOOL` | `TRUE` for PE32+ (64-bit image) |
| `is_dll` | `BOOL` | `TRUE` if `IMAGE_FILE_DLL` is set in FileHeader.Characteristics |
| `is_mapped` | `BOOL` | `TRUE` if the source buffer is a loaded image, `FALSE` if raw file |
| `sections_count` | `DWORD` | Number of section header entries |
| `imports_rva` | `DWORD` | Cached RVA of the import data directory |
| `import_size` | `DWORD` | Cached size of the import data directory |

### `SND_PE_MIN_FILE_ALIGNMENT`

**Value:** `512`

The minimum valid file alignment factor allowed for a PE image. Used to sanity-check PE structures against malformed headers.

---

## Utilities (`sindri/parsers/pe/pe_utils.h`)

### `snd_pe_compatibility_check`

Validates that a payload's bitness matches the host process architecture. This validation should occur before allocating memory or mapping sections.

| Parameter | Type | Description |
|---|---|---|
| `is_64bit` | `BOOL` | `TRUE` if the payload is 64-bit |

**Returns:** `BOOL` — `TRUE` if compatible, `FALSE` on mismatch.

---

### `snd_pe_get_entry_point`

Computes the absolute entry point address from `OptionalHeader.AddressOfEntryPoint` relative to the parser's source base.

| Parameter | Type | Description |
|---|---|---|
| `parser` | `const snd_pe_parser_t*` | Parsed PE context |

**Returns:** `void*` — absolute entry point address, or `NULL` if the RVA is zero or invalid.

---

### `snd_pe_get_tls_callbacks`

Retrieves the TLS callback array pointer from the TLS data directory. Returns `NULL` if no TLS directory is present.

| Parameter | Type | Description |
|---|---|---|
| `base_address` | `PVOID` | Base address of the mapped image |
| `parser` | `const snd_pe_parser_t*` | Parsed PE context |

**Returns:** `PVOID` — pointer to the null-terminated `PIMAGE_TLS_CALLBACK` array, or `NULL`.

---

### `snd_pe_execute_tls_callbacks`

Iterates the TLS callback array and calls each entry with the specified reason.

| Parameter | Type | Description |
|---|---|---|
| `virtual_base` | `PVOID` | Base address of the mapped image |
| `parser` | `const snd_pe_parser_t*` | Parsed PE context |
| `reason` | `DWORD` | Callback reason (e.g., `DLL_PROCESS_ATTACH`, `DLL_PROCESS_DETACH`) |

**Returns:** `snd_status_t` — `SND_OK` on success (including when no TLS directory exists).

---

### `snd_pe_rva_to_ptr`

Translates an RVA to an absolute pointer within the parser's tracked buffer. Performs a bounds check on `(rva, size)` before returning. For raw files (`is_mapped = FALSE`), performs section-table-based translation. For mapped images (`is_mapped = TRUE`), returns `source.base + rva` directly.

| Parameter | Type | Description |
|---|---|---|
| `parser` | `const snd_pe_parser_t*` | Parsed PE context |
| `rva` | `DWORD` | Relative Virtual Address to translate |
| `size` | `SIZE_T` | Expected data size at the target address (used for bounds checking) |

**Returns:** `PVOID` — pointer to the data, or `NULL` if out-of-bounds or the RVA is invalid.

---

### `snd_pe_get_directory`

Safely retrieves a data directory entry by index from the Optional Header's `DataDirectory` array.

| Parameter | Type | Description |
|---|---|---|
| `parser` | `const snd_pe_parser_t*` | Parsed PE context |
| `index` | `DWORD` | Directory index (e.g., `IMAGE_DIRECTORY_ENTRY_IMPORT`, `IMAGE_DIRECTORY_ENTRY_EXPORT`) |
| `dir_out` | `IMAGE_DATA_DIRECTORY*` | Output structure populated with `VirtualAddress` and `Size` |

**Returns:** `snd_status_t` — `SND_OK` on success, `SND_STATUS_DIRECTORY_NOT_FOUND` if the directory index is out of range or the directory's `VirtualAddress` is zero.

---

## Export Resolution (`sindri/parsers/pe/pe_exports.h`)

### `snd_pe_get_export_address`

Resolves an exported symbol by name from a mapped PE image. Handles export forwarders recursively via the `resolver` callback.

| Parameter | Type | Description |
|---|---|---|
| `base_address` | `PVOID` | Base address of the mapped PE image |
| `size` | `SIZE_T` | Size of the mapped PE image (for bounds checking) |
| `func_name` | `const char*` | Null-terminated export name to resolve |
| `func_addr_out` | `FARPROC*` | Receives the resolved function address |
| `resolver` | `snd_module_resolver_cb` | Callback to resolve external module bases for forwarders; pass `NULL` to fail on forwarders |

**Returns:** `snd_status_t` — `SND_OK` on success, `SND_STATUS_EXPORT_NOT_FOUND` if the name is not in the export table, `SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED` if a forwarder is encountered and `resolver` is `NULL`.

---

### `snd_pe_get_export_address_by_hash`

Hash-based variant of export resolution. Eliminates plaintext function name strings from the binary entirely.

| Parameter | Type | Description |
|---|---|---|
| `base_address` | `PVOID` | Base address of the mapped PE image |
| `size` | `SIZE_T` | Size of the mapped PE image |
| `func_hash` | `DWORD` | Pre-computed hash of the target export name |
| `func_addr_out` | `FARPROC*` | Receives the resolved function address |
| `resolver` | `snd_module_resolver_hash_cb` | Hash-based resolver callback for forwarder modules |

**Returns:** `snd_status_t` — `SND_OK` on success, or an export resolution error.

---

### `SND_FWD_MAX_DEPTH`

**Value:** `4`

Maximum recursion depth for export forwarder resolution. Prevents infinite loops from cyclically forwarded exports.

---

## Import Resolution (`sindri/parsers/pe/pe_imports.h`)

### `snd_pe_resolve_imports`

Walks the Import Descriptor table of a mapped PE and patches the IAT. For each descriptor: loads the dependency module via `mod_api->load_library`, then iterates the thunk array resolving by ordinal or name via `mod_api->get_proc_address`.

| Parameter | Type | Description |
|---|---|---|
| `base_address` | `PVOID` | Base address of the mapped PE image |
| `mod_api` | `const snd_module_api_t*` | Module API used for all loading and symbol resolution |
| `parser` | `const snd_pe_parser_t*` | Parsed PE context |

**Returns:** `snd_status_t` — `SND_OK` on success, or an import resolution error (`SND_STATUS_IMPORT_DLL_LOAD_FAILED`, `SND_STATUS_IMPORT_SYMBOL_RESOLVE_FAILED`, etc.).

---

### Ordinal Detection Macros

| Macro | Description |
|---|---|
| `SND_PE_SNAP_BY_ORDINAL32(ordinal)` | `TRUE` if the high bit of a 32-bit thunk value is set (ordinal-based import) |
| `SND_PE_SNAP_BY_ORDINAL64(ordinal)` | `TRUE` if the high bit of a 64-bit thunk value is set |
| `SND_PE_ORDINAL(ordinal)` | Extracts the 16-bit ordinal value from a thunk entry |

---

## Base Relocations (`sindri/parsers/pe/pe_relocations.h`)

### `snd_pe_apply_relocations`

Iterates the Base Relocation Table (`.reloc` section) and patches all hardcoded absolute addresses to align with the actual runtime base address. Each entry is bounds-checked before the write is applied.

The function computes `delta = actual_base - preferred_base`. If `delta` is zero (the image landed at its preferred `ImageBase`), the function returns immediately with `SND_OK`.

> [!NOTE]
> The memory at `base_address` must have at least `PAGE_READWRITE` permissions before calling this function, as it physically overwrites pointer-sized values inside `.text` and `.data` sections.

| Parameter | Type | Description |
|---|---|---|
| `base_address` | `PVOID` | Actual runtime base address where the PE image is mapped |
| `delta_offset` | `LONG_PTR` | Difference between actual base and preferred `ImageBase`; zero triggers early exit |
| `parser` | `const snd_pe_parser_t*` | Validated PE parser context |

**Returns:** `snd_status_t` — `SND_OK` on success, or `SND_STATUS_RELOCS_STRIPPED` if `IMAGE_FILE_RELOCS_STRIPPED` is set in the PE characteristics.

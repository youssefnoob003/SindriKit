# PE Parser: API Reference

Public PE parser API under `include/sindri/parsers/pe/`. Include `sindri/parsers/pe.h` or `sindri/parsers.h` for the full surface.

---

## Core Parser (`sindri/parsers/pe/parser.h`)

### `snd_pe_parse`

Validates and parses a raw or mapped PE buffer into a `snd_pe_parser_t` context. Checks the DOS signature (`MZ`), NT signature (`PE\0\0`), and Optional Header magic (`0x10B` PE32, `0x20B` PE32+).

| Parameter | Type | Description |
|---|---|---|
| `buf` | `const snd_buffer_t*` | Buffer containing PE data (pointer + size) |
| `is_mapped` | `BOOL` | `FALSE` for raw file layout, `TRUE` for virtually mapped image |
| `parser` | `snd_pe_parser_t*` | Output context to populate |

**Returns:** `snd_status_t` — `SND_OK` or a parse error (`SND_STATUS_INVALID_DOS_SIGNATURE`, `SND_STATUS_DOS_HEADER_TRUNCATED`, `SND_STATUS_NT_HEADERS_TRUNCATED`, `SND_STATUS_UNSUPPORTED_OPTIONAL_HEADER_MAGIC`, etc.)

**Source:** `src/parsers/pe/parser.c`

---

### `snd_pe_parser_t`

| Field | Type | Description |
|---|---|---|
| `source` | `snd_buffer_t` | Backing buffer (base pointer + tracked size) |
| `dos` | `PIMAGE_DOS_HEADER` | DOS header pointer into `source` |
| `nt.nt32` | `PIMAGE_NT_HEADERS32` | Valid when `is_64bit == FALSE` |
| `nt.nt64` | `PIMAGE_NT_HEADERS64` | Valid when `is_64bit == TRUE` |
| `section_head` | `PIMAGE_SECTION_HEADER` | First section header entry |
| `string_table` | `BYTE*` | COFF string table (raw files only) |
| `is_64bit` | `BOOL` | `TRUE` for PE32+ |
| `is_dll` | `BOOL` | `TRUE` if `IMAGE_FILE_DLL` is set |
| `is_mapped` | `BOOL` | Layout mode passed to `snd_pe_parse` |
| `sections_count` | `DWORD` | Number of section headers |
| `imports_rva` | `DWORD` | Cached import directory RVA |
| `import_size` | `DWORD` | Cached import directory size |

---

### Macros

| Macro | Value | Description |
|---|---|---|
| `SND_PE_GET_NT_FIELD(parser, field)` | — | Reads an Optional Header field without bitness branching |
| `SND_SYS_DLL_SIZE_DEFAULT` | `0x1000` | Bootstrap bound for in-memory system DLL parsing |

---

## Utilities (`sindri/parsers/pe/utils.h`)

| Macro | Value | Description |
|---|---|---|
| `SND_PE_MIN_FILE_ALIGNMENT` | `512` | Minimum valid file alignment (RVA translation) |

### `snd_pe_compatibility_check`

Validates payload bitness against the host process.

| Parameter | Description |
|---|---|
| `is_64bit` | `TRUE` if the payload is 64-bit |

**Returns:** `BOOL` — `TRUE` if compatible

---

### `snd_pe_get_entry_point`

Computes the absolute entry point from `AddressOfEntryPoint` via `snd_pe_rva_to_ptr`.

**Returns:** `void*` — entry point address, or `NULL`

---

### `snd_pe_get_tls_callbacks`

Retrieves the TLS callback array pointer from the TLS data directory.

| Parameter | Description |
|---|---|
| `base_address` | Runtime base of the mapped image |
| `parser` | Parsed PE context |

**Returns:** `PVOID` — callback array pointer, or `NULL`

---

### `snd_pe_execute_tls_callbacks`

Invokes each TLS callback with the specified reason. Requires `parser->is_mapped == TRUE`.

| Parameter | Description |
|---|---|
| `virtual_base` | Mapped image base |
| `parser` | Parsed PE context |
| `reason` | e.g. `DLL_PROCESS_ATTACH` |

**Returns:** `SND_OK` (including when no TLS directory exists), `SND_STATUS_ARCH_MISMATCH`, `SND_STATUS_UNSUPPORTED`

---

### `snd_pe_rva_to_ptr`

Translates an RVA to a pointer within the parser's tracked buffer with bounds checking.

| Parameter | Description |
|---|---|
| `parser` | Parsed PE context |
| `rva` | Relative Virtual Address |
| `size` | Expected data size at the target (for bounds check) |

**Returns:** `PVOID` — data pointer, or `NULL` if out of bounds

For mapped images: `source.base + rva`. For raw files: section-table translation with file alignment handling.

---

### `snd_pe_get_directory`

Safely retrieves a data directory entry by index.

| Parameter | Description |
|---|---|
| `index` | e.g. `IMAGE_DIRECTORY_ENTRY_IMPORT`, `IMAGE_DIRECTORY_ENTRY_EXPORT` |
| `dir_out` | Output `IMAGE_DATA_DIRECTORY` |

**Returns:** `SND_OK`, or `SND_STATUS_DIRECTORY_NOT_FOUND`

---

## Export Resolution (`sindri/parsers/pe/exports.h`)

Both export functions share a unified implementation and the **same forwarder resolver type** (`snd_module_resolver_cb`).

### `snd_pe_get_export_address`

Resolves an export by name or ordinal.

| Parameter | Type | Description |
|---|---|---|
| `base_address` | `PVOID` | Base of the mapped PE image |
| `size` | `SIZE_T` | Image size for bounds checking; pass `SND_SYS_DLL_SIZE_DEFAULT` to auto-expand from headers |
| `func_name` | `const char*` | Export name, or ordinal via `MAKEINTRESOURCE(n)` |
| `func_addr_out` | `FARPROC*` | Receives resolved address |
| `resolver` | `snd_module_resolver_cb` | Wide-name module lookup for forwarders; `NULL` fails forwarders |

**Returns:** `SND_OK`, `SND_STATUS_EXPORT_NOT_FOUND`, `SND_STATUS_EXPORT_DIRECTORY_INVALID`, `SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED`

---

### `snd_pe_get_export_address_hash`

Hash-based export resolution using compile-time hashes from `sindri_hashes.h`.

| Parameter | Type | Description |
|---|---|---|
| `func_hash` | `DWORD` | Pre-computed hash of the export name |
| `resolver` | `snd_module_resolver_cb` | Same wide-name resolver used by the string variant |

All other parameters match `snd_pe_get_export_address`.

---

### `SND_FWD_MAX_DEPTH`

**Value:** `4` — maximum recursion depth for export forwarder chains.

---

## Import Resolution (`sindri/parsers/pe/imports.h`)

### `snd_pe_resolve_imports`

Walks the Import Descriptor table and patches the IAT. Requires a mapped image.

| Parameter | Description |
|---|---|
| `base_address` | Runtime base of the mapped image |
| `mod_api` | Module API with `load_library` and `get_proc_address` populated |
| `parser` | Parsed PE context with `is_mapped == TRUE` |

**Returns:** `SND_OK`, or import errors (`SND_STATUS_IMPORT_DLL_LOAD_FAILED`, `SND_STATUS_IMPORT_SYMBOL_RESOLVE_FAILED`, `SND_STATUS_IMPORT_THUNK_INVALID`, etc.)

---

### Ordinal Macros

| Macro | Description |
|---|---|
| `SND_PE_SNAP_BY_ORDINAL32(ordinal)` | High bit set on 32-bit thunk |
| `SND_PE_SNAP_BY_ORDINAL64(ordinal)` | High bit set on 64-bit thunk |
| `SND_PE_ORDINAL(ordinal)` | Extracts 16-bit ordinal value |

---

## Base Relocations (`sindri/parsers/pe/relocations.h`)

### `snd_pe_apply_relocations`

Patches absolute addresses in a mapped image by `delta_offset`. Early-exits when `delta_offset == 0`.

| Parameter | Description |
|---|---|
| `base_address` | Actual runtime base |
| `delta_offset` | `actual_base - preferred ImageBase` |
| `parser` | Parsed context with `is_mapped == TRUE` |

**Returns:** `SND_OK`, `SND_STATUS_RELOCATION_DIRECTORY_INVALID`, `SND_STATUS_RELOCATION_PATCH_OUT_OF_RANGE`, `SND_STATUS_RELOCATION_TYPE_UNSUPPORTED`

---

## Section Utils (`sindri/parsers/pe/section_utils.h`)

| Function | Notes |
|---|---|
| `snd_pe_section_name` | Resolves section names including COFF string table |
| `snd_pe_section_copy_size` | Raw-to-virtual copy size |
| `snd_pe_section_loaded_size` | Final virtual section size |

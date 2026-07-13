# COFF Parser: API Reference

Public COFF parser API under `include/sindri/parsers/coff/`. Include `sindri/parsers/coff.h` or `sindri/parsers.h` for the full surface.

---

## Core Parser (`sindri/parsers/coff/parser.h`)

### `snd_coff_parse`

Parses a raw COFF object file buffer into a `snd_coff_parser_t` context.

| Parameter | Type | Description |
| --- | --- | --- |
| `buf` | `const snd_buffer_t*` | Raw buffer containing the COFF file data |
| `parser` | `snd_coff_parser_t*` | Output context to populate |

**Returns:** `snd_status_t` — `SND_OK` or a contextual error status.

---

### `snd_coff_parser_t`

Context structure for the COFF parser. Pointers point directly into the memory space backed by the `source` buffer; they do not own memory.

| Field | Type | Description |
| --- | --- | --- |
| `source` | `snd_buffer_t` | Backing buffer (base pointer + tracked size) |
| `file_header` | `PIMAGE_FILE_HEADER` | Pointer to the COFF file header |
| `section_head` | `PIMAGE_SECTION_HEADER` | First section header entry |
| `symbol_table` | `PIMAGE_SYMBOL` | Pointer to the symbol table |
| `symbol_count` | `DWORD` | Number of symbols in the symbol table |
| `string_table` | `BYTE*` | Pointer to the string table |
| `string_table_size` | `DWORD` | Size of the string table in bytes |
| `is_64bit` | `BOOL` | `TRUE` for 64-bit architectures (e.g. AMD64) |
| `sections_count` | `DWORD` | Number of sections |

---

## Utilities (`sindri/parsers/coff/utils.h`)

### `snd_coff_get_symbol_name`

Retrieves the actual name of a COFF symbol, automatically abstracting inline names (<= 8 chars) and long names stored in the String Table.

| Parameter | Type | Description |
| --- | --- | --- |
| `parser` | `const snd_coff_parser_t*` | Parsed COFF context |
| `symbol` | `PIMAGE_SYMBOL` | Symbol structure to evaluate |
| `name_out` | `char*` | Output buffer to store the null-terminated name |
| `name_len` | `SIZE_T` | Size of the output buffer |

**Returns:** `snd_status_t` — `SND_OK` on success.

---

### `snd_coff_get_section_name`

Retrieves the actual name of a COFF section, handling long names that reference the string table.

| Parameter | Type | Description |
| --- | --- | --- |
| `parser` | `const snd_coff_parser_t*` | Parsed COFF context |
| `section` | `PIMAGE_SECTION_HEADER` | Section header |
| `name_out` | `char*` | Output buffer to store the null-terminated name |
| `name_len` | `SIZE_T` | Size of the output buffer |

**Returns:** `snd_status_t` — `SND_OK` on success.

---

### `snd_coff_raw_to_ptr`

Abstracted address translation. Converts a raw file offset to a pointer with bounds checking.

| Parameter | Type | Description |
| --- | --- | --- |
| `parser` | `const snd_coff_parser_t*` | Parsed COFF context |
| `raw_offset` | `DWORD` | Raw file offset (e.g. `PointerToRawData`) |
| `size` | `SIZE_T` | Size of the expected data for bounds checking |

**Returns:** `PVOID` — Direct pointer to the data, or `NULL` if out of bounds.

---

### `snd_coff_write_jmp_trampoline`

Writes an x64 absolute jump trampoline (`jmp [rip+0]`) safely to a target destination, avoiding strict aliasing or alignment violations.

| Parameter | Type | Description |
| --- | --- | --- |
| `dest` | `void*` | Destination buffer where the trampoline will be written (min 14 bytes) |
| `target_addr` | `void*` | Absolute target address to jump to |

**Returns:** `void`

---

## Symbols (`sindri/parsers/coff/symbols.h`)

### `snd_coff_get_symbol_by_index`

Retrieves a symbol by its zero-based index in the symbol table. Note: Symbols can have auxiliary records which take up index slots.

| Parameter | Type | Description |
| --- | --- | --- |
| `parser` | `const snd_coff_parser_t*` | Parsed COFF context |
| `index` | `DWORD` | Zero-based index |

**Returns:** `PIMAGE_SYMBOL` — Pointer to the `IMAGE_SYMBOL`, or `NULL` if invalid/out of bounds.

---

### `snd_coff_find_symbol_by_name`

Finds a symbol by its exact string name.

| Parameter | Type | Description |
| --- | --- | --- |
| `parser` | `const snd_coff_parser_t*` | Parsed COFF context |
| `name` | `const char*` | Name to search for (e.g. "_MyFunction") |
| `symbol_out` | `PIMAGE_SYMBOL*` | Output pointer to store the located symbol |
| `index_out` | `DWORD*` | Optional output pointer to store the symbol's index |

**Returns:** `snd_status_t` — `SND_OK` on success, `SND_STATUS_NOT_FOUND`, or other errors.

---

### `snd_coff_decode_symbol`

Decodes and classifies a COFF symbol into its respective runtime category (Import, BSS, Local, or Other), resolving raw sizes and parsing DLL/function names for imports.

| Parameter | Type | Description |
| --- | --- | --- |
| `parser` | `const snd_coff_parser_t*` | Parsed COFF context |
| `sym` | `PIMAGE_SYMBOL` | Raw symbol structure to evaluate |
| `decoded` | `snd_coff_decoded_sym_t*` | Output structure to populate with decoded metadata |

**Returns:** `snd_status_t` — `SND_OK` on success, `SND_STATUS_INTEGER_OVERFLOW` on safe math checks, or parsing errors.

---

## Relocations (`sindri/parsers/coff/relocations.h`)

### `snd_coff_get_relocations`

Retrieves the array of relocations for a specific section.

| Parameter | Type | Description |
| --- | --- | --- |
| `parser` | `const snd_coff_parser_t*` | Parsed COFF context |
| `section` | `const IMAGE_SECTION_HEADER*` | Section header to retrieve relocations for |
| `relocations_out` | `PIMAGE_RELOCATION*` | Output pointer to the start of the `IMAGE_RELOCATION` array |
| `count_out` | `DWORD*` | Output pointer for the number of relocations |

**Returns:** `snd_status_t` — `SND_OK` on success (even if section has zero relocations), or other truncation/bounds errors.

---

### `snd_coff_apply_relocations`

Performs base relocations across the loaded sections of the COFF image, adjusting addresses natively in memory.

| Parameter | Type | Description |
| --- | --- | --- |
| `parser` | `const snd_coff_parser_t*` | Parsed COFF context |
| `section_map` | `LPVOID*` | Array of 1-based target mapped section base addresses |
| `symbol_map` | `LPVOID*` | Array of target resolved symbol addresses |
| `execution_base` | `LPVOID` | Base execution address used to compute relative targets (e.g., ADDR32NB) |

**Returns:** `snd_status_t` — `SND_OK` on success, `SND_STATUS_COFF_RELOCATION_UNSUPPORTED`, or parsing errors.

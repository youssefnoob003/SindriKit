# COFF Parser: Techniques

The COFF parser provides the structural interpretation necessary to handle raw Common Object File Format files. It is the foundation for executing unlinked object files, such as Beacon Object Files (BOFs), in-memory.

**Canonical reference:** [PE Format Specification (COFF Object Files)](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#coff-file-header-object-and-image)

---

## COFF Format Overview

A COFF object file is a raw, unlinked intermediate build artifact. Unlike PE files, it lacks an NT header (`PE\0\0`) and an Optional Header. It is structured simply as a file header followed by section headers, raw section data, relocations, symbols, and a string table:

```text
[ IMAGE_FILE_HEADER        ]  Machine, NumberOfSections, PointerToSymbolTable, etc.
[ IMAGE_SECTION_HEADER[]   ]  One per section: Name, PointerToRawData, PointerToRelocations
[ Section Data             ]  Raw bytes (.text, .data, .rdata, etc.)
[ Relocation Data          ]  IMAGE_RELOCATION arrays for each section
[ Symbol Table             ]  IMAGE_SYMBOL array
[ String Table             ]  DWORD size followed by null-terminated strings
```

COFF objects use raw file offsets (`PointerToRawData`, `PointerToRelocations`, etc.) rather than RVAs because they are not meant to be mapped into virtual memory directly.

---

## Zero-Copy Parsing Model

`snd_coff_parse` performs **zero-copy parsing**. Pointers inside `snd_coff_parser_t` (`file_header`, `section_head`, `symbol_table`, `string_table`) point directly into the caller's buffer. No heap allocations occur inside the parser.

The parser context lifecycle is strictly bound to the backing `snd_buffer_t`. If the buffer is freed or relocated, all cached pointers become invalid.

---

## The `snd_coff_parser_t` Context

`snd_coff_parse` populates a context that caches critical pointers:

| Field | Description |
| --- | --- |
| `source` | Copy of the input buffer descriptor (base + size) |
| `file_header` | Pointer to `IMAGE_FILE_HEADER` inside `source` |
| `section_head` | Pointer to the first `IMAGE_SECTION_HEADER` |
| `symbol_table` | Pointer to the `IMAGE_SYMBOL` array |
| `string_table` | Pointer to the string table data |
| `is_64bit` | `TRUE` if `Machine` is `IMAGE_FILE_MACHINE_AMD64` |

---

## Bounds Checking Philosophy

Every access to raw file offsets routes through `snd_coff_raw_to_ptr`, which validates that the requested range lies entirely within the tracked buffer. Violations return `NULL`.

Header parsing additionally uses bounds checks from the common buffer layer during the bootstrap parse in `snd_coff_parse` to ensure the file header and section table are fully contained. Relocation boundaries and BSS sizes are strictly evaluated to prevent integer overflow wrap-arounds during absolute memory calculation.

---

## String Table and Names

COFF symbols and sections store names inline if they are 8 characters or shorter. If a name is longer than 8 characters, the inline field stores an offset into the string table.

### Symbols

The `IMAGE_SYMBOL.N.Name.Short` is an 8-byte array. If `Short.Name.ShortName` starts with 4 null bytes (which it naturally does if it's an offset), the second DWORD (`Short.Name.LongName`) is an offset into the string table.

Valid string table offsets must be **greater than or equal to 4**, as the first 4 bytes of the string table contain the table's total size.

Use `snd_coff_get_symbol_name` to retrieve the actual name. It seamlessly resolves inline strings and string table offsets, copying the result into a caller-provided buffer while strictly enforcing COFF size boundaries.

### Sections

Section names follow a similar pattern but are slightly different. Long section names are typically represented as a forward slash followed by a decimal number (e.g., `/4`), where the number is the offset into the string table.

Use `snd_coff_get_section_name` to resolve section names cleanly, which inherently rejects malformed base64 encodings or illegal offsets (`< 4`).

---

## Symbol Table Iteration

The COFF symbol table is an array of `IMAGE_SYMBOL` structures. However, symbols can have auxiliary records (e.g. `IMAGE_AUX_SYMBOL`) immediately following them, which take up slots in the symbol table.

When iterating the symbol table manually, you must skip over auxiliary records by adding `symbol->NumberOfAuxSymbols` to your loop index.

`snd_coff_find_symbol_by_name` handles this correctly internally, efficiently finding symbols by string name without becoming misaligned by auxiliary records. External symbols, imports, and uninitialized `.bss` data are then classified via `snd_coff_decode_symbol`.

---

## Relocations

Unlike PE base relocations, which patch addresses assuming the image is relocated as a whole, COFF relocations link references between independent sections and external symbols.

Each section has its own array of `IMAGE_RELOCATION` structures, located at `PointerToRelocations`.

### Extended Relocations

If a section contains more than 65,535 relocations, `NumberOfRelocations` will be set to `0xFFFF` (`IMAGE_SCN_LNK_NRELOC_OVFL`). In this case, the actual 32-bit relocation count is stored in the `VirtualAddress` field of the *first* relocation record, and the real relocations begin at index 1.

Use `snd_coff_get_relocations` to abstract this complexity. It automatically handles extended relocation headers, performs bounds checking, and retrieves a pointer to the valid array and count:

```c
PIMAGE_RELOCATION relocations = NULL;
DWORD count = 0;
if (SND_SUCCEEDED(snd_coff_get_relocations(parser, section, &relocations, &count))) {
    if (count > 0) {
        // Process section relocations safely
    }
}
```

Handling these relocations involves looking up the symbol referenced by `SymbolTableIndex` (with explicit bounds checking against `symbol_count`) and computing the patched value based on the relocation `Type` (e.g. `IMAGE_REL_AMD64_ADDR32NB`, `IMAGE_REL_AMD64_REL32`).

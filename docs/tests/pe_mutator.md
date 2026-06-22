# PE Mutation Engine (`tests/loader/pe_mutator.py`)

**Location:** `tests/loader/pe_mutator.py`

The PE mutation engine applies targeted structural modifications to valid PE files to produce variants that stress-test the reflective loader's parser and loading pipeline. Every mutation is designed to exercise a specific safety boundary or parsing assumption.

## Mutation Categories

Mutations are divided into two strict categories:

### Benign Edge-Cases

These produce PE files that are unusual or hyper-optimized but structurally valid — Windows can still load and run them. The reflective loader **must** adapt and succeed.

| Mutation | Description | What It Tests |
|---|---|---|
| `unaligned_sections` | Forces `SectionAlignment` and `FileAlignment` to `0x200` | Loaders that hardcode `0x1000` page alignment assumptions |
| `strip_relocs_from_exe` | Strips the `.reloc` directory and sets `IMAGE_FILE_RELOCS_STRIPPED` | Loaders that aggressively demand relocations for EXEs |
| `garbage_dos_stub` | Overwrites the DOS stub (bytes 2–`e_lfanew`) with deterministic garbage | Parsers that read linearly through the DOS stub instead of seeking via `e_lfanew` |
| `append_overlay` | Appends 4 KB of junk data after the last section | Loaders that map beyond declared section boundaries |
| `null_section_names` | Zeroes out every section name string | Loaders that match section names unsafely or log via unchecked `printf` |

### Breaking Stressors

These produce structurally corrupted PE files. The parser **must** detect the violation and reject the input gracefully — **never crash**.

| Mutation | Description | What It Tests |
|---|---|---|
| `corrupt_reloc_block_size` | Sets first reloc block's `SizeOfBlock` to `0xFFFFFFFF` | Unbounded `while` loops in relocation parsing |
| `massive_size_of_headers` | Sets `SizeOfHeaders` to `0xFFFFFFFF` | Unchecked `memcpy` using header-sourced sizes |
| `section_truncated_raw_data` | Inflates a section's `SizeOfRawData` beyond the physical file | Blind section mapping without file bounds checks |
| `section_integer_overflow` | Sets `VirtualSize` and `SizeOfRawData` to `0xFFFFFFFF` | Integer overflow in allocation arithmetic (`VA + ALIGN(VirtualSize)`) |
| `section_overlap_corruption` | Collapses all section file offsets to zero | File alignment normalization bugs |
| `corrupt_pe_signature` | Replaces `PE\0\0` with `0xDEADBEEF` | NT signature validation |
| `invalid_e_lfanew` | Points `e_lfanew` 1 MB past EOF | Offset bounds checking before NT headers access |
| `zero_size_of_image` | Sets `SizeOfImage` to 0 | Allocation size validation |
| `corrupt_import_rva` | Points import directory to `0xDEAD0000` | RVA bounds checking before import traversal |
| `corrupt_export_rva` | Points export directory to `0xDEAD0000` | RVA bounds checking before export parsing |
| `huge_sections_count` | Sets `NumberOfSections` to `0xFFFF` | Section table overflow protection |
| `unterminated_imports` | Overwrites the null-terminating import descriptor with `0xFF` | Import loop termination without boundary checks |
| `massive_export_count` | Sets `NumberOfFunctions` and `NumberOfNames` to `0xFFFFFFFF` | Export iteration bounds checking |
| `oob_entry_point` | Points `AddressOfEntryPoint` past `SizeOfImage` | Entry point RVA validation before execution transfer |

## API

### `Mutation` Dataclass

| Field | Type | Description |
|---|---|---|
| `name` | `str` | Short string identifier (used in output filenames) |
| `description` | `str` | Human-readable label |
| `apply` | `Callable` | The mutation function (`(src, dst) -> None`) |
| `expect_loadable` | `bool` | `True` = loader must succeed; `False` = loader must reject gracefully |
| `applies_to` | `str` | `"both"`, `"exe"`, or `"dll"` |

### `mutate_pe(src_path, mutation, output_dir) -> str`

Applies the specified mutation to the source PE file and writes the mutated output to `output_dir`. Returns the path to the generated file. Raises `MutationError` on failure.

## Dependency

Requires the `pefile` Python package for structured PE modifications:
```
pip install pefile
```

Raw byte-level mutations (`garbage_dos_stub`, `corrupt_pe_signature`, `invalid_e_lfanew`, `huge_sections_count`) operate directly on the byte buffer without `pefile`.

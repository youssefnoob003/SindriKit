# PE Mutation Engine (`tests/loader/pe_mutator.py`)

**Location:** `tests/loader/pe_mutator.py`

Applies targeted structural modifications to valid PE files to stress-test the reflective loader parser and pipeline. Output filenames use each mutation's **`name`** field: `{basename}_{name}.dll`.

## Mutation categories

### Benign edge-cases

Structurally valid but unusual — the loader **should** succeed.

| `name` | Description | What it tests |
|---|---|---|
| `unaligned_sec` | `SectionAlignment` / `FileAlignment` → `0x200` | Hardcoded `0x1000` alignment assumptions |
| `stripped_relocs` | Strips `.reloc`, sets `IMAGE_FILE_RELOCS_STRIPPED` (EXE only) | Loaders that require relocations for EXEs |
| `garbage_dos_stub` | Overwrites DOS stub bytes with garbage | Linear DOS-stub reads vs `e_lfanew` seek |
| `append_overlay` | 4 KB junk after last section | Mapping beyond declared sections |
| `null_sec_names` | Zeroes section name strings | Unsafe section name handling |

### Breaking stressors

Structurally corrupt — parser **must reject gracefully**, never crash.

| `name` | Description | What it tests |
|---|---|---|
| `oob_reloc_size` | First reloc block `SizeOfBlock` = `0xFFFFFFFF` | Unbounded relocation loops |
| `massive_headers` | `SizeOfHeaders` = `0xFFFFFFFF` | Unchecked header-sized copies |
| `truncated_sec` | Section `SizeOfRawData` beyond file end | Section mapping bounds |
| `int_overflow` | `VirtualSize` / `SizeOfRawData` = `0xFFFFFFFF` | Allocation overflow arithmetic |
| `overlap_sections` | All section file offsets collapsed to zero | File alignment normalization |
| `corrupt_pe_sig` | NT signature replaced with `0xDEADBEEF` | PE signature validation |
| `bad_e_lfanew` | `e_lfanew` points past EOF | NT header offset bounds |
| `zero_imgsize` | `SizeOfImage` = 0 | Allocation size validation |
| `bad_import` | Import directory RVA → unmapped region | Import RVA bounds |
| `bad_export` | Export directory RVA → unmapped region (DLL only) | Export RVA bounds |
| `huge_sections` | `NumberOfSections` = `0xFFFF` | Section table overflow |
| `unterminated_imports` | Import descriptor terminator overwritten | Import loop termination |
| `massive_exports` | `NumberOfFunctions` / `NumberOfNames` = `0xFFFFFFFF` (DLL only) | Export iteration bounds |
| `oob_entry_point` | Entry RVA past `SizeOfImage` | Entry point validation |

## API

### `Mutation` dataclass

| Field | Type | Description |
|---|---|---|
| `name` | `str` | Short identifier (used in output filenames) |
| `description` | `str` | Human-readable label |
| `apply` | `Callable` | `(src, dst) -> None` mutation function |
| `expect_loadable` | `bool` | `True` = loader must succeed; `False` = must reject |
| `applies_to` | `str` | `"both"`, `"exe"`, or `"dll"` |

### `mutate_pe(src_path, mutation, output_dir) -> str`

Applies the mutation and writes `{name}_{mutation.name}{ext}` under `output_dir`. Raises `MutationError` on failure.

## Dependency

Requires `pefile` for structured edits:

```bash
pip install pefile
```

Raw byte mutations (`garbage_dos_stub`, `corrupt_pe_sig`, `bad_e_lfanew`, `huge_sections`) operate on the byte buffer directly.

## Related documentation

- [Test runner](test_runner.md) — `--mutate` flag
- [Test payloads](test_payloads.md)

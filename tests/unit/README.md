# Unit Tests

Host-side tests for the pure, CRT-friendly surface of SindriKit: status
encoding/strings, bounds and string helpers, hashing, the hijack frame painter,
input guards, and table-driven PE/COFF parser tests over synthetic in-memory
images built by the fixture builders (`pe_builder.c`, `coff_builder.c`).

Windows/MSVC only (the engine is `_WIN32`/`_WIN64`). Build with
`SND_BUILD_UNIT_TESTS=ON` (or `build.bat tests`) and run:

```bat
ctest --test-dir build64 -C Release --output-on-failure
:: or directly, for filtering:
build64\tests\unit\Release\snd_unit_tests.exe --list
build64\tests\unit\Release\snd_unit_tests.exe --filter "pe exports"
```

## Structure

- `test_framework.[ch]` — dependency-free registration + `--list` / `--filter`
  harness and the `SND_CHECK*` assertion macros. Unknown options and test
  registration failures are hard errors.
- `pe_builder.[ch]` — in-memory PE32/PE32+ builder (raw file layout or aligned
  image layout) with a single section; tests drop export/import/reloc/TLS
  blobs into it and point data directories at them.
- `coff_builder.[ch]` — in-memory COFF object builder (section table, packed
  symbol table, 32-bit string table).
- `test_pe_parser.c`, `test_pe_exports.c`, `test_pe_imports.c`,
  `test_pe_relocs.c`, `test_coff_parser.c`, `test_coff_symbols.c` — table-driven
  parser tests: happy paths for both bitnesses and both layout modes plus every
  documented rejection status.
- `test_inject_chains.c` — the classic/APC/hijack chain orchestrators driven
  through injected fake process/thread API tables: call ordering, argument
  marshalling, stage progression, error propagation and cleanup without a real
  process. The hijack success path is architecture-gated (x86 is an expected
  `SND_STATUS_ARCH_MISMATCH` rejection).
- `test_guards.c`, `test_hijack.c`, `test_main.c` — guard regressions, the
  hijack frame painter, and the pure common helpers.

`ctest` registers one entry per framework group (`--filter` prefix) plus the
full suite, so a fault in one group does not hide the rest. The process exit
code is 0 only if every selected test passed; the framework reports a per-test
count so a partial failure is never a green run.

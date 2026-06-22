# Loaders: Techniques

The loaders domain implements two distinct mechanisms for getting code into memory: Reflective PE Loading, which maps and executes a full PE image entirely in-process, and KnownDlls Mapping, which retrieves a clean, unhooked system DLL directly from the Windows Object Manager. Both are foundational to SindriKit's bootstrapping model and are built on top of the primitives domain's injected OS API tables.

---

## Reflective PE Loading

Reflective loading is the technique of mapping and executing a PE image (EXE or DLL) entirely in memory without invoking the Windows loader. No file touches disk during execution. The loaded image does not appear in the PEB's `InMemoryOrderModuleList`, leaving no `LoadLibrary` trace detectable via standard loader telemetry.

The concept was first published by Stephen Fewer in 2008 and has since become a foundational primitive in virtually all serious in-memory execution frameworks.

**Original research:** [Reflective DLL Injection](https://github.com/stephenfewer/ReflectiveDLLInjection) by Stephen Fewer (2008)

### The Loading Pipeline

SindriKit's reflective loader is entirely decoupled from the Win32 API. Every step operates through the `mem_api` and `mod_api` pointers injected into the `snd_loader_ctx_t` at initialization time. Swapping those pointers changes the entire footprint — from fully Win32-visible to completely syscall-direct — without touching a single line of loader logic.

The pipeline proceeds through the following stages, each tracked by `snd_loader_stage_t`:

#### 1. Parse (`SND_STAGE_PARSED`)
The raw payload buffer is passed to the PE parser (`snd_pe_parse`). The parser validates the DOS signature (`MZ`), the NT signature (`PE\0\0`), and the Optional Header magic (`0x10B` for PE32, `0x20B` for PE32+). On success, the parsed context — NT headers pointer, section table pointer, `is_64bit` flag — is stored inside `ctx.pe`.

#### 2. Architecture Check
`snd_compatibility_check` validates that the payload's bitness matches the host process. A 64-bit payload in a 32-bit process is rejected with `SND_STATUS_ARCH_MISMATCH` before any allocation occurs.

#### 3. Allocate and Copy Sections (`SND_STAGE_MEM_ALLOCATED`, `SND_STAGE_SECTIONS_MAPPED`)
`snd_allocate_and_copy_image` calls `ctx->mem_api->alloc` to reserve `SizeOfImage` bytes. It then copies each section from the raw file layout (using `PointerToRawData` as the source offset) to its virtual destination (using `VirtualAddress` as the target offset). Sections not present in the file (zero `SizeOfRawData`) are zeroed in the mapped image.

#### 4. Apply Base Relocations (`SND_STAGE_RELOCATED`)
`snd_apply_relocations` computes the delta between the actual allocated base and the PE's preferred `ImageBase`. It walks `IMAGE_BASE_RELOCATION` blocks in the `.reloc` section and, for each entry of type `IMAGE_REL_BASED_DIR64` (x64) or `IMAGE_REL_BASED_HIGHLOW` (x86), adds the delta to the embedded pointer. Each patch is bounds-checked before application.

If the image was allocated at its preferred base (delta is zero), this step is a no-op.

#### 5. Resolve Imports (`SND_STAGE_IMPORTS_RESOLVED`)
`snd_resolve_imports` walks the `IMAGE_IMPORT_DESCRIPTOR` table. For each descriptor, it calls `ctx->mod_api->load_library` to load the required DLL, then iterates the Import Name Table (INT). Each thunk is resolved: ordinal-based imports use the ordinal directly; name-based imports call `ctx->mod_api->get_proc_address`. Resolved addresses are written into the Import Address Table (IAT).

#### 6. Apply Memory Protections (`SND_STAGE_READY_FOR_EXECUTION`)
`snd_apply_memory_protections` iterates sections and calls `ctx->mem_api->protect` to set per-section page protections based on `Characteristics` flags:
- `IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ` → `PAGE_EXECUTE_READ`
- `IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE` → `PAGE_READWRITE`
- `IMAGE_SCN_MEM_READ` only → `PAGE_READONLY`

No section is mapped `PAGE_EXECUTE_READWRITE` unless explicitly demanded by the image's section flags, minimizing memory scanner surface.

#### 7. TLS Callbacks
`snd_execute_tls_callbacks` checks for a TLS data directory. If present, it iterates the `PIMAGE_TLS_CALLBACK` array and calls each callback with the virtual base and the specified reason (e.g., `DLL_PROCESS_ATTACH`).

#### 8. Execute (`SND_STAGE_EXECUTED`)
`snd_execute_reflective_image` resolves the entry point (`snd_get_entry_point`) and calls it. For DLLs, the entry point is `DllMain(hModule, DLL_PROCESS_ATTACH, NULL)`. For EXEs, it is the `AddressOfEntryPoint`-derived function.

---

## KnownDlls Mapping

When operating in native mode, SindriKit needs a clean `ntdll.dll` to extract SSNs from. The PEB-resident copy is the exact target of EDR userland hooking — its stubs may already be patched. Retrieving a fresh file from disk is detectable. The Windows Object Manager provides a cleaner alternative.

At boot, the Session Manager (`smss.exe`) creates pre-mapped, read-only section objects for core system DLLs under the `\KnownDlls` Object Manager directory. These sections are snapshots taken before any userland security software is initialized. Opening `\KnownDlls\ntdll.dll` via `NtOpenSection` and mapping it into the current process with `NtMapViewOfSection` yields a pristine image whose text section has never been touched by EDR hooks.

The base address of this clean mapping is then fed directly to `snd_set_ntdll()`, ensuring that all subsequent SSN resolution operate on unmodified stubs.

```c
PVOID clean_ntdll = NULL;
snd_status_t status = snd_map_knowndll(&snd_knowndlls_win, L"ntdll.dll", &clean_ntdll);
if (status.code == SND_SUCCESS) {
    snd_set_ntdll(clean_ntdll);
}
```

SindriKit exposes two configurations for the underlying section API calls:
- `snd_knowndlls_win`: uses Win32 wrappers — suitable for early bootstrapping before any syscall pipeline is configured.
- `snd_knowndlls_native`: uses raw NT functions directly — a fully Win32-free bootstrapping path once the syscall pipeline is ready.

> [!WARNING]
> `snd_map_knowndll()` requires a configuration struct (`&snd_knowndlls_win` or `&snd_knowndlls_native`) to dictate how it opens the section handles. If native mode is selected, the operator must ensure direct syscall primitives are manually bootstrapped elsewhere before attempting the map.

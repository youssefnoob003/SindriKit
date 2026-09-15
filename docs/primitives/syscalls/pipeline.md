# Syscall Execution Pipeline

EDRs hook `ntdll.dll` syscall stubs in userland. Direct syscalls skip those stubs: the operator resolves the System Service Number (SSN) for a target `Nt*` function and invokes `syscall` with that number. SindriKit supports both direct and indirect syscall invocation. Direct syscalls execute the `syscall` instruction inline, while indirect syscalls jump to a legitimate gadget within NTDLL to evade EDR call-stack analysis.

SSNs vary across Windows builds. SindriKit resolves them dynamically at runtime against a caller-supplied `ntdll` image.

---

## Lifecycle

```mermaid
flowchart LR
    A["1. snd_ntdll_set_clean"] --> B["2. snd_syscall_set_resolver / _add"]
    B --> C["3. snd_syscall_set_invoker"]
    C --> D["4. snd_syscall_set_spoof_finder"]
    D --> E["5. snd_syscall_invoke"]
```

### 1. Provide `ntdll` base

Register the image used for SSN extraction:

```c
snd_ntdll_set_clean(ntdll_base);
```

| Source | Trade-off |
|---|---|
| PEB-resident `ntdll` | No I/O; may reflect hooked stubs (scan falls back to neighbor search) |
| KnownDlls map | Clean text section; recommended for `_sys` backends |
| Disk load | Simple; file read telemetry |

See [mapping techniques](../mapping/internals.md) for KnownDlls bootstrap.

### 2. Configure strategy chain

```c
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
snd_syscall_set_invoker(snd_syscall_direct_invoke_asm);
// or for indirect syscalls:
// snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);
// snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);
```

`snd_syscall_set_resolver` **replaces** the entire chain. Each `snd_syscall_add_resolver` appends up to 3 fallbacks (4 total).

### 3. Configure invoker

```c
snd_syscall_set_invoker(snd_syscall_direct_invoke_asm);     // direct
// or
snd_syscall_set_invoker(snd_syscall_indirect_invoke_asm);   // indirect
snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan); // required for indirect
```

Indirect invocation requires a gadget finder. `snd_syscall_find_gadget_scan` resolves the target function in the natively loaded NTDLL via PEB and scans for a `syscall; ret` gadget (x64) or the transition stub entry (x86).

### 4. Configure spoof finder (optional)

If using `snd_syscall_spoofed_invoke_asm`, you must configure a spoof finder. It dynamicallly searches for a "Fat Frame" in `kernel32.dll` to hide the call stack.

```c
snd_syscall_set_spoof_finder(snd_syscall_find_spoof_scan);
```

### 5. Resolve SSN

```c
snd_syscall_entry_t entry = {0};
snd_status_t status = snd_syscall_resolve(SND_HASH_NTOPENSECTION, &entry);
```

`snd_syscall_invoke` internally calls `snd_syscall_resolve` to try each registered strategy in order until one returns `SND_OK`, then automatically populates `ssn` and `sys_addr` before dispatching.

### 6. Invoke

Populate `snd_syscall_args_t` and call `snd_syscall_invoke`:

```c
snd_syscall_args_t args = {0};
args.arg1             = ...;
// arg2–arg11 as required by the target syscall

NTSTATUS nt_status;
snd_status_t status = snd_syscall_invoke(SND_HASH_NTOPENSECTION, &args, &nt_status);
```

`_sys` primitive implementations (`snd_mem_sys`, `snd_proc_sys`, `snd_map_sys`) wrap step 5–6 internally — operators only bootstrap once at startup.

---

## Full example

```c
// Bootstrap (once per process)
PVOID ntdll = NULL;
snd_status_t st = snd_om_knowndll_map(&snd_map_nt, L"ntdll.dll", &ntdll);
if (SND_FAILED(st)) return st;

snd_ntdll_set_clean(ntdll);
snd_syscall_set_resolver(snd_syscall_resolve_ssn_scan);
snd_syscall_add_resolver(snd_syscall_resolve_ssn_sort);
snd_syscall_set_invoker(snd_syscall_spoofed_invoke_asm);
snd_syscall_set_gadget_finder(snd_syscall_find_gadget_scan);
snd_syscall_set_spoof_finder(snd_syscall_find_spoof_scan);

// Resolve + invoke NtClose
snd_syscall_args_t args = {0};
args.arg1 = handle;

NTSTATUS nt;
st = snd_syscall_invoke(SND_HASH_NTCLOSE, &args, &nt);
```

---

## `SND_USE_DEFAULTS`

When enabled, all globals (invoker, gadget finder, primary resolver) are pre-configured. The only required bootstrap call is `snd_ntdll_set_clean()`.

> [!TIP]
> **OpSec Rationale:** Why use a compile-time macro instead of just initializing the variables to default pointers in `syscalls.c`? 
> If the variables were unconditionally initialized with pointers to `snd_syscall_indirect_invoke_asm` and `snd_syscall_find_gadget_scan`, the C linker would be forced to pull those entire functions (including the scanner logic and ASM stubs) into the final compiled binary, even if the user explicitly chose to use direct syscalls or no syscalls at all. 
> By using `SND_USE_DEFAULTS`, we ensure the default dependency graph is completely severed when disabled, keeping the payload footprint as lean and evasive as possible.

---

## Integration with `_sys` backends

After bootstrap, swapping to syscall-backed primitives requires no additional setup:

```c
ctx.mem_api  = &snd_mem_sys;
inj_ctx.proc_api = &snd_proc_sys;
```

Each `_sys` API function calls `snd_syscall_invoke` with the appropriate hash and arguments. The `snd_syscall_invoke` function handles SSN resolution internally and dispatches to the configured invoker.

---

## Failure modes

| Status | Cause |
|---|---|
| `SND_STATUS_NTDLL_NOT_INITIALIZED` / `SND_STATUS_RESOLVER_NOT_INITIALIZED` | NTDLL base or resolver chain was not configured |
| `SND_STATUS_SSN_NOT_FOUND` | All strategies failed for the hash |
| `SND_STATUS_PIPELINE_EXHAUSTED` | Strategy chain full (max 4) |
| `SND_STATUS_NULL_POINTER` | NULL resolver passed to `strategy_add` |

---

## See also

- [Resolver engines](engines.md) — scan vs sort internals
- [API reference](../../api_reference.md)

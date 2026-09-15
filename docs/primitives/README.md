# Primitives Domain

Foundation layer for SindriKit. Loaders, injection, and future domains rely on injected OS API tables and execution bridges documented here.

> [!IMPORTANT]
> **Profile-aware OpSec:** Evasive profiles use PEB walking and hash-based resolution (`snd_mod_nt`, `_sys` backends). Diagnostic profiles use Win32 APIs (`snd_mod_win`). Match backends to your deployment tier.

## Subdomains

| Subdomain | Backends / focus |
|---|---|
| [memory/](memory/README.md) | `snd_mem_win`, `snd_mem_nt`, `snd_mem_sys` |
| [modules/](modules/README.md) | `snd_mod_win`, `snd_mod_nt` (no `_sys`) |
| [files/](files/README.md) | `snd_file_win`, `snd_file_nt`, `snd_file_sys` |
| [mapping/](mapping/README.md) | `snd_map_win`, `snd_map_nt`, `snd_map_sys`, KnownDlls |
| [process/](process/README.md) | `snd_proc_win`, `snd_proc_nt`, `snd_proc_sys` |
| [thread/](../../include/sindri/primitives/thread.h) | `snd_thread_win`, `snd_thread_nt`, `snd_thread_sys` (queue APC, resume, suspend) |
| [syscalls/](syscalls/README.md) | SSN resolution pipeline, configurable invoker (direct / indirect / spoofed) |
| [execution/](execution/README.md) | FFI (`snd_ffi_execute`), Heaven's Gate |

Contract definitions: `include/sindri/primitives/os_api.h`  
Umbrella include: `include/sindri/primitives.h`

## Table of Contents

- [execution/](execution/README.md) — dynamic FFI, WoW64 transition; syscall ASM co-located in source
- [memory/](memory/README.md) — local virtual memory (`win`, `nt`, `sys`)
- [modules/](modules/README.md) — local module load and export resolution
- [files/](files/README.md) — local file loading and IO
- [mapping/](mapping/README.md) — section mapping and KnownDlls bootstrap
- [process/](process/README.md) — remote process operations (injection consumer)
- [thread/](../api_reference.md#snd_thread_api_t) — APC queue, resume, suspend (injection consumer; see process api_reference)
- [syscalls/](syscalls/README.md) — direct kernel invocation, cascading SSN resolvers

## Related documentation

- [Architecture: dependency injection](../architecture/dependency_injection.md)
- [Parsers domain](../parsers/README.md)

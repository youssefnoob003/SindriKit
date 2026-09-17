# Parsers Domain

In-memory interpretation of Windows file formats and runtime environment structures. Split into **PE**, **COFF**, and **Env** subdomains mirroring `include/sindri/parsers/`.

All three are included by `sindri/parsers.h`.

> [!IMPORTANT]
> Parsers are **read-only interpreters**: they validate and return pointers into the caller's buffer, and never allocate, patch, or load. Mutation lives in the loader engines — `snd_ldr_pe_apply_relocations` and `snd_ldr_pe_resolve_imports` apply base relocations and patch the IAT using parser getters such as `snd_pe_get_reloc_block` and `snd_pe_get_import_name`.

## Subdomains

| Subdomain | Umbrella | Purpose |
|---|---|---|
| [pe/](pe/README.md) | `sindri/parsers/pe.h` | PE headers, exports, imports, relocations, TLS |
| [coff/](coff/README.md) | `sindri/parsers/coff.h` | COFF headers, sections, symbols, relocations |
| [env/](env/README.md) | `sindri/parsers/env.h` | PEB module walking, local PEB access |

## How parsers fit the framework

```
                    ┌─────────────────────────────────────┐
                    │  Domains (loaders, primitives)      │
                    └──────────────┬──────────────────────┘
                                   │
              ┌────────────────────┼────────────────────┐
              ▼                    ▼                    ▼
     snd_pe_parse            snd_pe_get_export_*   snd_peb_get_module_*
              │                    │                    │
              └────────────────────┴────────────────────┘
                                   │
                          Parsers (PE + Env ...)
```

- **Reflective loaders** — validate, relocate, resolve imports, TLS
- **NT/syscall primitives** — resolve `Nt*` from `ntdll` without `GetProcAddress`
- **`snd_mod_nt`** — delegates PEB walk and EAT parse to env + PE parsers

## Table of Contents

### PE
- [pe/README.md](pe/README.md) — overview and header map
- [pe/internals.md](pe/internals.md) — format, bounds, export/import/reloc mechanics
- [api_reference.md](../api_reference.md) — full public PE API

### COFF
- [coff/README.md](coff/README.md) — overview and header map
- [coff/internals.md](coff/internals.md) — format, symbols, relocations, string table handling
- [api_reference.md](../api_reference.md) — full public COFF API

### Env
- [env/README.md](env/README.md) — overview and scope
- [env/internals.md](env/internals.md) — PEB layout, module walking
- [api_reference.md](../api_reference.md) — full public env API

## Related documentation

- [Loaders domain](../loaders/README.md)
- [Modules primitives](../primitives/modules/README.md)

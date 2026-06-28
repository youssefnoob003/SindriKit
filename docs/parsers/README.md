# Parsers Domain

In-memory interpretation of Windows file formats and runtime environment structures. Split into **PE** and **Env** subdomains mirroring `include/sindri/parsers/`.

Both are included by `sindri/parsers.h`.

> [!IMPORTANT]
> Parsers are **read-only interpreters** except where explicitly documented (e.g. `snd_pe_apply_relocations`, `snd_pe_resolve_imports` write into a caller-owned mapped image). They do not load DLLs or allocate OS resources.

## Subdomains

| Subdomain | Umbrella | Purpose |
|---|---|---|
| [pe/](pe/) | `sindri/parsers/pe.h` | PE headers, exports, imports, relocations, TLS |
| [env/](env/) | `sindri/parsers/env.h` | PEB module walking, local PEB access |

## How parsers fit the framework

```
                    ┌─────────────────────────────────────┐
                    │  Domains (loaders, primitives)     │
                    └──────────────┬──────────────────────┘
                                   │
              ┌────────────────────┼────────────────────┐
              ▼                    ▼                    ▼
     snd_pe_parse            snd_pe_get_export_*   snd_peb_get_module_*
              │                    │                    │
              └────────────────────┴────────────────────┘
                                   │
                          Parsers (PE + Env)
```

- **Reflective loaders** — validate, relocate, resolve imports, TLS
- **NT/syscall primitives** — resolve `Nt*` from `ntdll` without `GetProcAddress`
- **`snd_mod_nt`** — delegates PEB walk and EAT parse to env + PE parsers

## Table of Contents

### PE
- [pe/README.md](pe/README.md) — overview and header map
- [pe/techniques.md](pe/techniques.md) — format, bounds, export/import/reloc mechanics
- [pe/api_reference.md](pe/api_reference.md) — full public PE API

### Env
- [env/README.md](env/README.md) — overview and scope
- [env/techniques.md](env/techniques.md) — PEB layout, module walking
- [env/api_reference.md](env/api_reference.md) — full public env API

## Related documentation

- [Loaders domain](../domains/loaders/README.md)
- [Modules primitives](../domains/primitives/modules/README.md)

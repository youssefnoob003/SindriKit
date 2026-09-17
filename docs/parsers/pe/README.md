# PE Parser

In-memory Portable Executable parsing. Public headers under `include/sindri/parsers/pe/`, aggregated by `include/sindri/parsers/pe.h`.

## Header map

| Header | Role |
|---|---|
| `parser.h` | Parser struct, bootstrap constants |
| `utils.h` | RVA translation, data directories, entry point, TLS, page-protection flags |
| `exports.h` | EAT resolution (name, hash, ordinal, forwarders) |
| `imports.h` | Import descriptor and thunk walking (read-only) |
| `relocations.h` | Base relocation block/entry retrieval (read-only) |
| `section.h` | Section-name and section-size helpers used by loaders |

## Source map

| Source | Implements |
|---|---|
| `src/parsers/pe/parser.c` | Initial PE parsing |
| `src/parsers/pe/utils.c` | RVA, directories, TLS, entry point |
| `src/parsers/pe/exports.c` | Unified export resolver |
| `src/parsers/pe/imports.c` | Import descriptor, name, and thunk traversal |
| `src/parsers/pe/relocations.c` | Relocation block/entry parsing (application lives in the PE loader) |
| `src/parsers/pe/section.c` | Section helpers |

## Table of Contents

- [internals.md](internals.md) — PE format, bounds model, export/import/reloc mechanics
- [api_reference.md](../../api_reference.md) — full public PE API

## Related documentation

- [Parsers domain](../README.md)
- [Env parser](../env/README.md) — PEB walking for forwarders and `snd_mod_nt`

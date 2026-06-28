# PE Parser

In-memory Portable Executable parsing. Public headers under `include/sindri/parsers/pe/`, aggregated by `include/sindri/parsers/pe.h`.

## Header map

| Header | Role |
|---|---|
| `parser.h` | `snd_pe_parse`, `snd_pe_parser_t`, bootstrap constants |
| `utils.h` | RVA translation, data directories, entry point, TLS, architecture check |
| `exports.h` | EAT resolution (name, hash, ordinal, forwarders) |
| `imports.h` | Import descriptor walk and IAT patching |
| `relocations.h` | Base relocation application |
| `section_utils.h` | Internal section helpers (loader engine use) |

## Source map

| Source | Implements |
|---|---|
| `src/parsers/pe/parser.c` | `snd_pe_parse` |
| `src/parsers/pe/utils.c` | RVA, directories, TLS, entry point |
| `src/parsers/pe/exports.c` | Unified export resolver |
| `src/parsers/pe/imports.c` | Import resolution |
| `src/parsers/pe/relocations.c` | Relocation patching |
| `src/parsers/pe/section_utils.c` | Section helpers |

## Table of Contents

- [techniques.md](techniques.md) — PE format, bounds model, export/import/reloc mechanics
- [api_reference.md](api_reference.md) — full public PE API

## Related documentation

- [Parsers domain](../README.md)
- [Env parser](../env/README.md) — PEB walking for forwarders and `snd_mod_nt`

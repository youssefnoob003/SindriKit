# COFF Parser

In-memory Common Object File Format (COFF) parsing. Public headers under `include/sindri/parsers/coff/`, aggregated by `include/sindri/parsers/coff.h`.

## Header map

| Header | Role |
|---|---|
| `parser.h` | `snd_coff_parse`, `snd_coff_parser_t` |
| `utils.h` | Pointer translation, section/symbol name resolution |
| `symbols.h` | Symbol retrieval by index and name |
| `relocations.h` | Base relocation retrieval for sections |

## Source map

| Source | Implements |
|---|---|
| `src/parsers/coff/parser.c` | `snd_coff_parse` |
| `src/parsers/coff/utils.c` | Pointer translation, name string resolution |
| `src/parsers/coff/symbols.c` | Symbol table lookup |
| `src/parsers/coff/relocations.c` | Section relocation retrieval |

## Table of Contents

- [techniques.md](techniques.md) — COFF format, string table mechanics, bounds model
- [api_reference.md](api_reference.md) — full public COFF API

## Related documentation

- [Parsers domain](../README.md)
- [PE parser](../pe/README.md) — parsing mapped/executable PE images

# Examples & Proof of Concepts

Standalone executables under `pocs/` demonstrate SindriKit domains end-to-end. Each PoC is a thin CLI wrapper around library chain functions — the same APIs documented in the domain references.

PoCs build when `SND_CRTLESS=OFF` (default). CRT-less mode replaces the full PoC set with `loader_noCRT_nowinapi` only.

Each PoC walkthrough follows: **Location → What it demonstrates → (Command-line) → Walkthrough → Building → OpSec → See also**.

## OpSec profiles

| PoC | Loader / injection | Memory | Modules | Process | Syscall bootstrap |
|---|---|---|---|---|---|
| `loader_winapi` | Local reflective | `snd_mem_win` | `snd_mod_win` | — | Optional |
| `loader_nowinapi` | Local reflective | `snd_mem_nt` | `snd_mod_nt` | — | Required (disk `ntdll`) |
| `loader_noCRT_nowinapi` | Local reflective | `snd_mem_win` | `snd_mod_nt` | — | Required (PEB `ntdll`) |
| `inject_shell` | Classic shellcode | — | — | `snd_proc_win` | Yes (KnownDlls; unused while `_win`) |
| `inject_pe` | Classic PE | `snd_mem_sys` | `snd_mod_nt` | `snd_proc_sys` | Required (KnownDlls) |
| `heavens_gate` | WoW64 → x64 exec | Win32 alloc (demo) | — | — | N/A |

## Table of Contents

### Loaders (local reflective PE)
- [loader_winapi.md](loader_winapi.md) — Win32 APIs, diagnostic baseline, DLL export FFI
- [loader_nowinapi.md](loader_nowinapi.md) — NT backends, syscall pipeline bootstrap
- [loader_noCRT_nowinapi.md](loader_noCRT_nowinapi.md) — `/NODEFAULTLIB`, minimal CRT-less footprint

### Injection (remote classic)
- [inject_shell.md](inject_shell.md) — Raw shellcode via `snd_inj_classic_shell`
- [inject_pe.md](inject_pe.md) — PE bake + remote execute via `snd_inj_classic_pe`

### Execution primitives
- [heavens_gate.md](heavens_gate.md) — WoW64 → native x64 transition

## Building all PoCs

```bash
cmake -B build -DSND_BUILD_PAYLOADS=ON
cmake --build build --config Release
```

Binaries land under `build/pocs/<name>/Release/` (MSVC multi-config) or `build/pocs/<name>/` (single-config generators).

CRT-less PoC only:

```bash
cmake -B build -DSND_CRTLESS=ON -DSND_ENABLE_DEBUG=OFF
cmake --build build --config Release
```

## Related documentation

- [Getting started: basic usage](../getting_started/basic_usage.md)
- [Loaders domain](../domains/loaders/README.md)
- [Injection domain](../domains/injection/README.md)

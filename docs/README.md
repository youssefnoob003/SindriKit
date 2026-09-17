# SindriKit Documentation

SindriKit is an embeddable static engine (`sindri::engine`) for building offensive capabilities. Start with the API reference or the getting-started guide, then drill into a domain.

> [!IMPORTANT]
> **Domain independence:** A loader must not call injection internals directly. Use documented chain APIs and shared primitive tables.

## Start here

| Page | Contents |
|---|---|
| [API reference](api_reference.md) | Canonical public C API — functions, types, macros, DI tables |
| [Getting started](getting_started/README.md) | Build flags, first build, syscall bootstrap, loader/injection workflow |
| [Architecture](architecture/README.md) | Dependency injection, state machines, status system, internal boundaries |
| [Examples](examples/README.md) | The `unified` PoC CLI and per-command walkthroughs |

## Domains

| Domain | Contents |
|---|---|
| [Primitives](primitives/README.md) | Injected OS API tables and execution bridges — memory, modules, process, mapping, files, thread, syscalls, execution, object manager |
| [Loaders](loaders/README.md) | In-memory payload bootstrapping — reflective PE and COFF/BOF |
| [Injection](injection/README.md) | Remote process injection — classic and APC, over shellcode/PE/COFF |
| [Parsers](parsers/README.md) | PE, COFF, and env (PEB/NTDLL) parsing |

## Reference & tooling

| Page | Contents |
|---|---|
| [Common](common/README.md) | CRT-free helpers — buffers, hashing, strings, macros, opcodes |
| [Status codes](status_codes.md) | Generated facility-encoded catalog |
| [Config](config/README.md) | `config/hashes.ini` hash manifest |
| [Scripts](scripts/README.md) | Hash generation and the mutation engine |
| [Tests](tests/README.md) | Loader integration runners and the PE mutator |

## Documentation layout

Every domain folder follows the same shape: `README.md` is the overview and header/API index, and the deep-dive page is `internals.md` when the folder covers a single mechanism. Folders that own several distinct mechanisms use topic-named pages instead — `primitives/execution/{ffi,heavens_gate}.md`, `primitives/syscalls/{pipeline,engines}.md`, `common/infrastructure.md`. Cross-cutting concepts live under `architecture/`; executable usage lives under `examples/`.

API signatures and per-domain status codes live in the [API reference](api_reference.md); hex values are in the generated [status codes](status_codes.md) table. Bootstrap recipes have one canonical home each — e.g. the syscall pipeline in [primitives/syscalls/pipeline.md](primitives/syscalls/pipeline.md) — and are linked rather than repeated.

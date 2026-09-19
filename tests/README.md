# Tests

Automated testing infrastructure for SindriKit. Four layers:

- **Unit** (`unit/`) — host-side tests of the pure, CRT-friendly engine surface:
  status encoding, bounds/string helpers, hashing, the hijack frame painter,
  table-driven PE/COFF parser tests over synthetic in-memory images, and the
  injection chain orchestrators driven through injected fake API tables.
  Windows/MSVC only. See [unit/README.md](unit/README.md).
- **Integration** (`loaders/`) — data-driven matrices that expand compact specs
  across every (backend × architecture) combination and drive the `unified`
  PoC: the PE loader, the COFF loader, and a PE mutation engine that
  stress-tests the parser against structurally malformed inputs.
- **Integration** (`injection/`) — the same matrix style for
  `unified inject classic|apc|hijack`, asserting a remote side effect (a marker
  file written by the injected payload) instead of process output.
- **Harness self-test** (`loaders/selftest_common.py`) — pure-Python tests of
  the shared runner infrastructure (`loaders/runner_core.py`). Runs on any host
  with Python 3; catches regressions in the harness itself (e.g. a dead
  expectation) without needing a Windows build tree.

## Shared runner infrastructure

`tests/loaders/runner_core.py` owns everything the matrices share: the
`TestCase` model, the pure `evaluate_test` pass/fail decision, build-tree
preflight, subprocess execution (`run_case`), arch/substring filtering, and the
strict-mode summary. The PE/COFF/injection runners are thin declarative files
that supply paths, backends, specs, and payloads.

## Quick reference

```bat
build.bat tests                              :: build fixtures + unit + PoCs (both arches)
ctest --test-dir build64 -C Release --output-on-failure
python tests/loaders/selftest_common.py      :: any host, no build tree required

python tests/loaders/pe/test_runner.py --mutate --strict --arch x64
python tests/loaders/coff/test_runner.py --strict --arch x64
python tests/injection/classic/test_runner.py --strict --arch x64
python tests/injection/apc/test_runner.py --strict --arch x64
python tests/injection/hijack/test_runner.py --strict --arch x64
```

All three runners accept `--strict` (CI mode): any skip — missing build tree,
missing mutation base, no tests selected — is a hard failure. Without a build
tree the runners exit non-zero rather than reporting an empty green run.

## Mutation expectation semantics

`pe_mutator.py` classifies mutations as **benign** (`expect_loadable = True`) or
**breaking** (`expect_loadable = False`), and the runner enforces that
classification:

- Benign mutations must load **and run**: the expected exit code and/or success
  marker must appear (e.g. `Export returned: 0x...` for DLLs, `Jumping to EXE
  Entry Point` for EXEs).
- Breaking mutations must be **rejected**: the process must exit non-zero and
  print the `[ERR]` rejection marker. A crash (`0xC0000005`) is never an
  acceptable rejection.

Before this was enforced, benign mutations that the loader wrongly rejected, and
breaking mutations the loader silently executed, both reported PASS.

`--mutate` requires `pefile` (`pip install -r requirements-dev.txt`); without it
the runner fails loudly instead of silently running zero mutation cases.

## Injection semantics

The injection suites mirror the loaders: one declarative runner per technique
(`injection/classic/`, `injection/apc/`, `injection/hijack/`) over the shared
`injection/inject_core.py` orchestration. Each builds a lightweight console
target plus PE/COFF marker payloads whose entry point writes
`snd_inject_probe.tmp` containing `SND_INJECT_OK` **into the target executable's
directory** (found via `GetModuleFileNameA(NULL)` in the target, so the location
does not depend on the target's working directory). After `unified inject`
returns, the runner polls for that file (injection runs asynchronously in the
target), then verifies its content:

- Exit code 0 + marker → PASS.
- Exit code 0 but no marker → FAIL ("reported success, no side effect").
- Non-zero exit → FAIL with the `[ERR]` output.
- x86 thread-hijack is an **expected rejection**: the entry-frame ABI marshals
  arguments on the stack there, so those cases assert a clean
  `SND_STATUS_ARCH_MISMATCH` rejection instead of a marker.

Classic injection uses `-p <pid>` (the runner spawns the target and passes its
PID); APC/hijack use `-t <image>` (the engine spawns the target suspended).
The target is always the console `test_inject_target_<arch>.exe` because
`NtCreateUserProcess` targets must not be GUI images (see AGENTS.md). Its idle
loop uses an **alertable** `SleepEx`, which is required for the Win32/sys APC
cases to deliver their queued APC after the target resumes.

The PE marker payload is deliberately **CRT-less** (custom entry point,
`/NODEFAULTLIB`): the chains start a thread at the entry point with no
arguments, so a normal `DllMainCRTStartup` would fail with
`STATUS_DLL_INIT_FAILED` (0xC0000142).

## Table of Contents
- [loaders/](loaders/) — PE/COFF suites, shared runner core, PE mutator, and
  their payload sources.
- [injection/](injection/) — target + marker fixtures and the injection matrix.
- [fixtures/](fixtures/) — pre-built binary assets (e.g. the Corkami PE corpus)
  used by the local-only `--corkami` stress mode. Corkami is **never** wired
  into CI.

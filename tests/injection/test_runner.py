"""Shared orchestration for the per-technique injection integration runners.

Mirrors the loader suites: each technique has a thin declarative runner
(`classic/`, `apc/`, `hijack/`) and this module owns what they share — fixture
paths, the marker side-effect protocol, target lifecycle, and the async
poll-for-marker verification.

The observable success signal is a marker file written by the injected payload
into the target executable's directory (discovered with GetModuleFileNameA in
the target, so it does not depend on the target's working directory).
"""

import argparse
import os
import subprocess
import sys
import time
from typing import Optional, Sequence, Tuple

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from loaders.runner_core import (BuildTree, Colors, REJECT_MARKER, TestCase, filter_cases,  # noqa: E402
                                 preflight, run_case, summarize, vt100_enable)

MARKER = "snd_inject_probe.tmp"
MARKER_CONTENT = "SND_INJECT_OK"
INJECT_TIMEOUT = 15.0
POLL_TIMEOUT = 5.0
POLL_INTERVAL = 0.1

ARCHES = ("x64", "x86")
_BITS = {"x64": 64, "x86": 32}
_BIN = {64: r"build64\pocs", 32: r"build32\pocs"}
_INJ = {64: r"build64\tests\injection", 32: r"build32\tests\injection"}

TREES = (BuildTree("x64", _BIN[64], _INJ[64]), BuildTree("x86", _BIN[32], _INJ[32]))

BACKENDS = (
    ("win", "Win32", ("--win",)),
    ("nt", "Native API", ("--nt",)),
    ("sys-direct-scan", "Syscalls (direct, scan)", ("--sys", "--invoke-direct", "--resolve-scan")),
    ("sys-indirect-scan", "Syscalls (indirect, scan)", ("--sys", "--invoke-indirect", "--resolve-scan")),
    ("sys-spoofed-scan", "Syscalls (spoofed, scan)", ("--sys", "--invoke-spoofed", "--resolve-scan")),
)

# Technique -> how the target is acquired.
SPAWN_PID = "pid"   # classic: the runner spawns the target and passes -p <pid>
SPAWN_IMG = "image"  # apc/hijack: the engine spawns the target via -t <image>


def loader(arch: str) -> str:
    return os.path.abspath(os.path.join(_BIN[_BITS[arch]], "Release", "unified.exe"))


def target_image(arch: str) -> str:
    return os.path.abspath(os.path.join(_INJ[_BITS[arch]], "Release", f"test_inject_target_{arch}.exe"))


def payload_path(arch: str, kind: str) -> str:
    if kind == "pe":
        return os.path.abspath(os.path.join(_INJ[_BITS[arch]], "Release", f"test_inject_marker_{arch}.dll"))
    return os.path.abspath(os.path.join(_INJ[_BITS[arch]], f"test_inject_marker_{arch}.obj"))


def marker_path(arch: str) -> str:
    return os.path.join(os.path.dirname(target_image(arch)), MARKER)


def make_case(technique: str, payload: str, backend_label: str, backend_args, arch: str, x86_reject: bool) -> TestCase:
    spawn = SPAWN_PID if technique == "classic" else SPAWN_IMG
    cmd = [loader(arch), "inject", technique, payload, "-f", payload_path(arch, payload)]
    cmd += ["-p", "{PID}"] if spawn == SPAWN_PID else ["-t", target_image(arch)]
    cmd += list(backend_args)

    reject = arch == "x86" and x86_reject
    return TestCase(
        name=f"{technique.title()} {payload.upper()} ({backend_label}) ({arch})",
        cmd=cmd,
        expect_fail=reject,
        expect_reject=REJECT_MARKER if reject else None,
        expect_output="Architecture incompatible with target payload" if reject else None,
    )


def _remove_marker(arch: str, retries: int = 50) -> bool:
    """Remove the marker, waiting for a terminated target to release it."""
    path = marker_path(arch)
    for _ in range(retries):
        try:
            os.remove(path)
            return True
        except FileNotFoundError:
            return True
        except PermissionError:
            time.sleep(POLL_INTERVAL)
        except OSError:
            return False
    return not os.path.exists(path)


def run_inject_case(tc: TestCase, known_missing: Optional[set]) -> Optional[bool]:
    arch = next(a for a in ARCHES if f"({a})" in tc.name)
    marker = marker_path(arch)
    spawned = {"proc": None}

    def pre_run() -> Optional[str]:
        if not _remove_marker(arch):
            return f"marker file is still locked: {marker}"
        target = target_image(arch)
        if not os.path.exists(target):
            return f"target image missing (run 'build.bat tests'): {target}"
        if "{PID}" in tc.cmd:
            proc = subprocess.Popen([target])
            spawned["proc"] = proc
            tc.cmd = [arg.replace("{PID}", str(proc.pid)) for arg in tc.cmd]
        return None

    def verify() -> Tuple[bool, Optional[str]]:
        deadline = time.monotonic() + POLL_TIMEOUT
        while time.monotonic() < deadline:
            if os.path.exists(marker):
                with open(marker, "r", encoding="utf-8", errors="replace") as f:
                    content = f.read().strip()
                if content == MARKER_CONTENT:
                    return True, None
                return False, f"marker content mismatch: {content!r}"
            time.sleep(POLL_INTERVAL)
        return False, f"marker file {marker!r} not produced within {POLL_TIMEOUT:.0f}s"

    def teardown() -> None:
        if spawned["proc"] is not None:
            spawned["proc"].kill()
            spawned["proc"].wait()
        try:
            subprocess.run(["taskkill", "/IM", f"test_inject_target_{arch}.exe", "/T", "/F"],
                           capture_output=True, text=True)
        except OSError:
            pass
        _remove_marker(arch)

    # Rejection cases, when present, assert a clean failure and produce no
    # marker, so skip the side-effect verification for them.
    verify_fn = None if tc.expect_reject is not None else verify

    return run_case(tc, TREES, known_missing=known_missing, timeout=INJECT_TIMEOUT,
                    pre_run=pre_run, verify=verify_fn, teardown=teardown)


def run_all(matrices: Sequence[Tuple[str, Sequence[Tuple[str, bool]]]]) -> None:
    """Run all injection techniques. `matrices` is (technique, specs) pairs."""
    parser = argparse.ArgumentParser(description="SindriKit unified injection test runner")
    parser.add_argument("--arch", action="append", choices=list(ARCHES),
                        help="Only run the given architecture (repeatable).")
    parser.add_argument("--exclude-substr", action="append", default=[], metavar="S",
                        help="Skip test cases whose name contains S (repeatable).")
    parser.add_argument("--strict", action="store_true",
                        help="Treat any skip / missing dependency as a hard failure (CI mode).")
    args = parser.parse_args()

    arches = tuple(args.arch or ARCHES)
    cases = []

    for technique, specs in matrices:
        for payload, x86_reject in specs:
            for _bname, blabel, bargs in BACKENDS:
                for arch in arches:
                    cases.append(make_case(technique, payload, blabel, bargs, arch, x86_reject))

    cases = filter_cases(cases, arches, args.exclude_substr)

    known_missing = preflight(TREES, arches)
    if args.strict and known_missing:
        print(f"[{Colors.RED}ERROR{Colors.RESET}] --strict: build trees missing: {sorted(known_missing)}")
        sys.exit(1)

    print(f"\n{Colors.BLUE}=== SindriKit Injection Tests ==={Colors.RESET}\n")

    passed = skipped = 0
    for tc in cases:
        result = run_inject_case(tc, known_missing)
        if result is True:
            passed += 1
        elif result is None:
            skipped += 1

    sys.exit(summarize(passed, skipped, len(cases), args.strict))

if __name__ == "__main__":
    vt100_enable()
    run_all([
        ("classic", [("pe", False), ("coff", False)]),
        ("apc",     [("pe", False), ("coff", False)]),
        ("hijack",  [("pe", False), ("coff", False)]),
    ])

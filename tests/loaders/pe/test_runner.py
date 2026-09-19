"""
SindriKit PE Loader Integration Test Runner.

Declarative matrix: a compact Spec expands into one TestCase per (backend, arch)
combination and is executed through the shared runner_core plumbing. Mutation
and Corkami cases are appended here because they are PE-specific.

Usage:
    python tests/loaders/pe/test_runner.py [--corkami] [--mutate] [--strict]
"""

import argparse
import os
import sys
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from runner_core import (BuildTree, Colors, REJECT_MARKER, TestCase, filter_cases, preflight, run_case,  # noqa: E402
                         summarize, vt100_enable)

try:
    import pe_mutator
except ImportError:
    pe_mutator = None

# ── Path Configuration ──────────────────────────────────────────────────────

BIN64_DIR = r"build64\pocs"
BIN32_DIR = r"build32\pocs"
TEST64_DIR = r"build64\tests\loaders\pe"
TEST32_DIR = r"build32\tests\loaders\pe"

CORKAMI_DIR = r"tests\fixtures\pe\corkami"
CORKAMI_ZIP = r"tests\fixtures\pe\corkami_fixtures.zip"

_BIN = {64: BIN64_DIR, 32: BIN32_DIR}
_TEST = {64: TEST64_DIR, 32: TEST32_DIR}
_BITS = {"x64": 64, "x86": 32}
_PTR_WIDTH = {"x64": 16, "x86": 8}  # MSVC %p hex-digit count

ARCHES = ("x64", "x86")

TREES = (BuildTree("x64", BIN64_DIR, TEST64_DIR), BuildTree("x86", BIN32_DIR, TEST32_DIR))

BACKENDS = (
    ("win", "Win32", ()),
    ("nt", "Native API", ("--nt",)),
    ("sys-direct-scan", "Syscalls (direct, scan)", ("--sys", "--invoke-direct", "--resolve-scan")),
    ("sys-indirect-scan", "Syscalls (indirect, scan)", ("--sys", "--invoke-indirect", "--resolve-scan")),
    ("sys-spoofed-scan", "Syscalls (spoofed, scan)", ("--sys", "--invoke-spoofed", "--resolve-scan")),
    ("sys-indirect-sort", "Syscalls (indirect, sort)", ("--sys", "--invoke-indirect", "--resolve-sort")),
)


# ── Spec (compact, auto-expanding) ──────────────────────────────────────────


@dataclass
class Spec:
    """Architecture- and backend-independent test specification."""

    backends: Tuple[str, ...]
    payload: str  # e.g. "test_dll", "test_exe_advanced"
    export: Optional[str] = None
    args: List[str] = field(default_factory=list)
    expect_output: Optional[str] = None  # static expected text
    expect_retval: Optional[int] = None  # expected FFI return (arch-formatted)
    expect_rc: Optional[int] = None  # expected process exit code
    expect_fail: bool = False
    label: str = ""

    def _ext(self) -> str:
        return ".dll" if "dll" in self.payload else ".exe"

    def _format_retval(self, arch: str) -> str:
        w = _PTR_WIDTH[arch]
        return f"Export returned: 0x{self.expect_retval:0{w}X}"

    def to_test_case(self, backend_label: str, backend_args: Tuple[str, ...], arch: str) -> TestCase:
        bits = _BITS[arch]
        loader_exe = os.path.abspath(os.path.join(_BIN[bits], "Release", "unified.exe"))
        payload_file = os.path.abspath(os.path.join(_TEST[bits], "Release", f"{self.payload}_{arch}{self._ext()}"))

        cmd = [loader_exe, "load", "pe", "-f", payload_file] + list(backend_args)
        if self.export:
            cmd += ["-e", self.export]
        for a in self.args:
            cmd += ["-a", a]

        expect = self.expect_output
        if self.expect_retval is not None:
            expect = self._format_retval(arch)

        return TestCase(
            name=f"{backend_label} ({arch}) -> {self.label}",
            cmd=cmd,
            expect_output=expect,
            expect_returncode=self.expect_rc,
            expect_fail=self.expect_fail,
        )


SPECS = [
    Spec(tuple(name for name, _, _ in BACKENDS), "test_dll", "SayHello", ["bonjour", "hello", "12"],
         expect_retval=0xFEEDC0DE, label="Load DLL with exact args"),
    Spec(tuple(name for name, _, _ in BACKENDS), "test_dll", "SayHello", ["wrong", "args"],
         expect_retval=0xDEADBEEF, label="Edge Case: Bad Args Validation"),
    Spec(tuple(name for name, _, _ in BACKENDS), "test_dll", expect_output="Export name is required for DLL payloads",
         expect_fail=True, label="Edge Case: Missing Export Parameter"),
    Spec(tuple(name for name, _, _ in BACKENDS), "test_dll_advanced", "AdvancedExport", ["advanced_test"],
         expect_retval=0x1337C0DE, label="Load Advanced DLL (Imports, Allocs)"),
    Spec(tuple(name for name, _, _ in BACKENDS), "test_dll_empty", "NonExistentExport",
         expect_output="Requested PE data directory entry is missing", expect_fail=True,
         label="Load Empty DLL (Missing Dirs)"),
    Spec(tuple(name for name, _, _ in BACKENDS), "test_dll", "VerifyInit", expect_retval=0xC001D00D,
         label="Verify DllMain + Relocations"),
    Spec(tuple(name for name, _, _ in BACKENDS), "test_dll_advanced", "VerifyImports", expect_retval=0xCA11AB1E,
         label="Verify Multi-Import Resolution"),
    Spec(tuple(name for name, _, _ in BACKENDS), "test_dll_tls", "VerifyTLS", expect_retval=0x71500C01,
         label="Verify TLS Callbacks"),
    Spec(tuple(name for name, _, _ in BACKENDS), "test_exe", expect_output="Jumping to EXE Entry Point", expect_rc=122,
         label="Run EXE"),
    Spec(tuple(name for name, _, _ in BACKENDS), "test_exe_advanced",
         expect_output="Successfully allocated and printed", expect_rc=4919, label="Run Advanced EXE (Stdlib Init)"),
]


def expand_specs(specs):
    """Expand Specs into TestCases."""
    cases = []
    for backend_name, backend_label, backend_args in BACKENDS:
        for arch in ARCHES:
            for spec in specs:
                if backend_name in spec.backends:
                    cases.append(spec.to_test_case(backend_label, backend_args, arch))
    return cases


# ── Arch-Mismatch Tests ────────────────────────────────────────────────────


def build_mismatch_tests():
    cases = []
    for loader_arch, payload_arch in [("x64", "x86"), ("x86", "x64")]:
        lb, pb = _BITS[loader_arch], _BITS[payload_arch]
        cases.append(
            TestCase(
                name=f"Win32 ({loader_arch}) -> Arch Mismatch Guard ({loader_arch} loader, {payload_arch} DLL)",
                cmd=[
                    os.path.abspath(os.path.join(_BIN[lb], "Release", "unified.exe")),
                    "load", "pe", "-f",
                    os.path.abspath(os.path.join(_TEST[pb], "Release", f"test_dll_{payload_arch}.dll")),
                    "-e", "SayHello",
                ],
                expect_output="Architecture incompatible with target payload",
                expect_fail=True,
            )
        )
    return cases


# ── Corkami Fuzz Tests (local only, never enforced in CI) ──────────────────


def load_corkami_tests(enabled=False):
    if not enabled:
        return []

    if not os.path.exists(CORKAMI_DIR) or not os.listdir(CORKAMI_DIR):
        print(f"\n[{Colors.RED}ERROR{Colors.RESET}] Corkami fixtures missing!")
        print(f"[*] Expected: {Colors.YELLOW}{CORKAMI_DIR}{Colors.RESET}")
        print(f"[*] Unzip '{CORKAMI_ZIP}' into that folder (password: infected).\n")
        sys.exit(1)

    return [
        TestCase(
            name=f"Corkami Parser Stress Test -> {f}",
            cmd=[os.path.abspath(os.path.join(_BIN[64], "Release", "unified.exe")), "load", "pe", "-f",
                 os.path.join(CORKAMI_DIR, f)],
            corkami_fuzz=True,
        )
        for f in sorted(os.listdir(CORKAMI_DIR))
        if f.lower().endswith(".exe")
    ]


# ── Mutation Engine Tests ───────────────────────────────────────────────────


def load_mutation_tests(enabled=False, selected_arches=(), excluded=()):
    """Generate mutated PEs and enforce `expect_loadable` strictly: benign
    mutations must load AND run; breaking mutations must be rejected with an
    explicit [ERR] marker and a non-zero exit. A crash is never acceptable."""
    if not enabled:
        return []

    if pe_mutator is None or pe_mutator.pefile is None:
        raise SystemExit(
            "ERROR: --mutate requires 'pefile' (pip install -r requirements-dev.txt). "
            "Refusing to silently run zero mutation cases."
        )

    print(f"\n[{Colors.BLUE}INFO{Colors.RESET}] Generating mutated PE variants...")
    tests = []

    for arch in selected_arches:
        bits = _BITS[arch]
        base_exe = os.path.join(_TEST[bits], "Release", f"test_exe_{arch}.exe")
        base_dll = os.path.join(_TEST[bits], "Release", f"test_dll_{arch}.dll")

        out_dir = os.path.join(f"build{bits}", "mutations")
        os.makedirs(out_dir, exist_ok=True)

        for _backend_name, backend_label, backend_args in BACKENDS:
            case_prefix = f"{backend_label} ({arch})"
            if any(substr in case_prefix for substr in excluded):
                continue
            loader_exe = os.path.join(_BIN[bits], "Release", "unified.exe")

            for mutation in pe_mutator.ALL_MUTATIONS:
                if mutation.applies_to == "dll":
                    src_file, cmd_args, loadable_stdout, loadable_rc = (
                        base_dll, ["-e", "SayHello"], "Export returned: 0x", None)
                else:
                    src_file, cmd_args, loadable_stdout, loadable_rc = (
                        base_exe, [], "Jumping to EXE Entry Point", 122)

                if not os.path.exists(src_file):
                    tests.append(TestCase(
                        name=f"{case_prefix} -> Mutation: {mutation.name}",
                        cmd=[],
                        setup_error=f"mutation base missing; run 'build.bat tests': {src_file}",
                    ))
                    continue

                try:
                    mutated_path = pe_mutator.mutate_pe(src_file, mutation, out_dir)
                except pe_mutator.MutationError as e:
                    tests.append(TestCase(
                        name=f"{case_prefix} -> Mutation: {mutation.name}",
                        cmd=[],
                        setup_error=str(e),
                    ))
                    continue

                if mutation.expect_loadable:
                    tests.append(TestCase(
                        name=f"{case_prefix} -> Mutation: {mutation.name}",
                        cmd=[loader_exe, "load", "pe", "-f", os.path.abspath(mutated_path)]
                        + list(backend_args) + cmd_args,
                        expect_output=loadable_stdout,
                        expect_returncode=loadable_rc,
                    ))
                else:
                    tests.append(TestCase(
                        name=f"{case_prefix} -> Mutation: {mutation.name}",
                        cmd=[loader_exe, "load", "pe", "-f", os.path.abspath(mutated_path)]
                        + list(backend_args) + cmd_args,
                        expect_fail=True,
                        corkami_fuzz=True,
                    ))
    return tests


# ── Entry Point ─────────────────────────────────────────────────────────────


def main():
    parser = argparse.ArgumentParser(description="SindriKit PE Integration Test Runner")
    parser.add_argument("--corkami", action="store_true",
                        help="Include Corkami malformed-PE stress tests (local only, not CI)")
    parser.add_argument("--mutate", action="store_true",
                        help="Generate and run dynamic PE mutations to stress-test the loader")
    parser.add_argument("--arch", action="append", choices=list(ARCHES),
                        help="Only run the given architecture (repeatable).")
    parser.add_argument("--exclude-substr", action="append", default=[], metavar="S",
                        help="Skip test cases whose name contains S (repeatable).")
    parser.add_argument("--strict", action="store_true",
                        help="Treat any skip / missing dependency as a hard failure (CI mode).")
    args = parser.parse_args()

    print("==================================================")
    print("         SindriKit Integration Tests              ")
    print("==================================================")

    selected_arches = tuple(args.arch or ARCHES)
    preflight_missing = preflight(TREES, selected_arches)
    if args.strict and preflight_missing:
        print(f"[{Colors.RED}ERROR{Colors.RESET}] --strict: build trees missing: {sorted(preflight_missing)}")
        sys.exit(1)

    full_matrix = (
        expand_specs(SPECS)
        + build_mismatch_tests()
        + load_corkami_tests(enabled=args.corkami)
        + load_mutation_tests(enabled=args.mutate, selected_arches=selected_arches, excluded=args.exclude_substr)
    )
    full_matrix = filter_cases(full_matrix, selected_arches, args.exclude_substr)

    print(f"[*] {len(full_matrix)} test cases ({len(SPECS)} specs x {len(BACKENDS)} backends x {len(ARCHES)} arches)\n")

    passed = skipped = 0
    for test in full_matrix:
        result = run_case(test, TREES, known_missing=preflight_missing)
        if result is True:
            passed += 1
        elif result is None:
            skipped += 1

    sys.exit(summarize(passed, skipped, len(full_matrix), args.strict))


if __name__ == "__main__":
    vt100_enable()
    main()
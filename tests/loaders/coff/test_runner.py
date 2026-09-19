"""
SindriKit COFF Loader Integration Test Runner.

Declarative matrix over (backend, arch) executed through the shared
runner_core plumbing.

Usage:
    python tests/loaders/coff/test_runner.py [--strict]
"""

import argparse
import os
import sys
from dataclasses import dataclass
from typing import Optional

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from runner_core import (BuildTree, Colors, TestCase, filter_cases, preflight, run_case,  # noqa: E402
                         summarize)

# ── Path Configuration ──────────────────────────────────────────────────────

BIN64_DIR = r"build64\pocs"
BIN32_DIR = r"build32\pocs"
TEST64_DIR = r"build64\tests\loaders\coff"
TEST32_DIR = r"build32\tests\loaders\coff"

_BIN = {64: BIN64_DIR, 32: BIN32_DIR}
_TEST = {64: TEST64_DIR, 32: TEST32_DIR}
_BITS = {"x64": 64, "x86": 32}

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


@dataclass
class Spec:
    payload: str
    entry: str
    args: str = ""
    expect_output: Optional[str] = None
    expect_fail: bool = False
    label: str = ""

    def to_test_case(self, backend_label: str, backend_args, arch: str) -> TestCase:
        bits = _BITS[arch]
        loader_exe = os.path.abspath(os.path.join(_BIN[bits], "Release", "unified.exe"))
        payload_obj = os.path.abspath(os.path.join(_TEST[bits], f"{self.payload}_{arch}.obj"))

        cmd = [loader_exe, "load", "coff", "-f", payload_obj, "-e", self.entry]
        cmd.extend(backend_args)
        if self.args:
            cmd.extend(["-a", self.args])

        return TestCase(
            name=f"{backend_label} ({arch}) -> {self.label or self.payload}",
            cmd=cmd,
            expect_output=self.expect_output,
            expect_fail=self.expect_fail,
        )


SPECS = [
    Spec(payload="test_coff_basic", entry="go",
         expect_output="COFF Basic Test: Execution successful.", label="Basic Execution"),
    Spec(payload="test_coff_reloc", entry="go",
         expect_output="COFF Reloc Test: Global DATA and BSS accessed successfully.",
         label="Internal Relocations (.data/.bss)"),
    Spec(payload="test_coff_args", entry="go", args="test_arguments_string",
         expect_output="COFF Args Test: Execution successful.", label="Argument Passing"),
    # Noninteractive USER32 import resolution: the payload calls GetDesktopWindow
    # and prints a marker after the call returns.
    Spec(payload="test_coff_user32", entry="go",
         expect_output="COFF USER32 Test: GetDesktopWindow resolved.", label="USER32 Symbol Resolution"),
]


def main():
    parser = argparse.ArgumentParser(description="SindriKit COFF Integration Test Runner")
    parser.add_argument("--arch", action="append", choices=list(ARCHES),
                        help="Only run the given architecture (repeatable).")
    parser.add_argument("--exclude-substr", action="append", default=[], metavar="S",
                        help="Skip test cases whose name contains S (repeatable).")
    parser.add_argument("--strict", action="store_true",
                        help="Treat any skip / missing dependency as a hard failure (CI mode).")
    args = parser.parse_args()

    print(f"\n{Colors.BLUE}=== SindriKit COFF Integration Tests ==={Colors.RESET}\n")

    cases = []
    for _backend_name, backend_label, backend_args in BACKENDS:
        for spec in SPECS:
            for arch in ARCHES:
                cases.append(spec.to_test_case(backend_label, backend_args, arch))

    selected_arches = tuple(args.arch or ARCHES)
    cases = filter_cases(cases, selected_arches, args.exclude_substr)

    known_missing = preflight(TREES, selected_arches)
    if args.strict and known_missing:
        print(f"[{Colors.RED}ERROR{Colors.RESET}] --strict: build trees missing: {sorted(known_missing)}")
        sys.exit(1)

    passed = skipped = 0
    for tc in cases:
        result = run_case(tc, TREES, known_missing=known_missing)
        if result is True:
            passed += 1
        elif result is None:
            skipped += 1

    sys.exit(summarize(passed, skipped, len(cases), args.strict))


if __name__ == "__main__":
    main()
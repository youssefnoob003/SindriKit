"""
SindriKit COFF Loader Test Runner
"""

import os
import subprocess
import sys
from dataclasses import dataclass
from typing import List, Optional, Tuple

# ── Path Configuration ──────────────────────────────────────────────────────

BIN64_DIR = r"build64\pocs"
BIN32_DIR = r"build32\pocs"
TEST64_DIR = r"build64\tests\loaders\coff"
TEST32_DIR = r"build32\tests\loaders\coff"

_BIN = {64: BIN64_DIR, 32: BIN32_DIR}
_TEST = {64: TEST64_DIR, 32: TEST32_DIR}
_BITS = {"x64": 64, "x86": 32}

ARCHES = ("x64", "x86")

BACKENDS = (
    ("win", "Win32", ()),
    ("nt", "Native API", ("--nt",)),
    ("sys-direct-scan", "Syscalls (direct, scan)", ("--sys", "--invoke-direct", "--resolve-scan")),
    ("sys-indirect-scan", "Syscalls (indirect, scan)", ("--sys", "--invoke-indirect", "--resolve-scan")),
    ("sys-spoofed-scan", "Syscalls (spoofed, scan)", ("--sys", "--invoke-spoofed", "--resolve-scan")),
    ("sys-indirect-sort", "Syscalls (indirect, sort)", ("--sys", "--invoke-indirect", "--resolve-sort")),
)

class Colors:
    GREEN = "\033[92m"
    RED = "\033[91m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    RESET = "\033[0m"

@dataclass
class TestCase:
    name: str
    cmd: List[str]
    expect_stdout: Optional[str] = None
    expect_fail: bool = False
    expect_returncode: Optional[int] = None

@dataclass
class Spec:
    payload: str
    entry: str
    args: str = ""
    expect_stdout: Optional[str] = None
    expect_fail: bool = False
    label: str = ""

    def to_test_case(self, backend_label: str, backend_args, arch: str) -> TestCase:
        bits = _BITS[arch]
        loader_exe = os.path.abspath(os.path.join(_BIN[bits], "Release", "unified.exe"))

        payload_obj = os.path.join(
            _TEST[bits], f"{self.payload}_{arch}.obj"
        )
        payload_obj = os.path.abspath(payload_obj)

        cmd = [loader_exe, "load", "coff", "-f", payload_obj, "-e", self.entry]
        cmd.extend(backend_args)
        if self.args:
            cmd.extend(["-a", self.args])

        return TestCase(
            name=f"{backend_label} ({arch}) -> {self.label or self.payload}",
            cmd=cmd,
            expect_stdout=self.expect_stdout,
            expect_fail=self.expect_fail,
        )

SPECS = [
    Spec(
        label="Basic Execution",
        payload="test_coff_basic",
        entry="go",
        expect_stdout="COFF Basic Test: Execution successful.",
    ),
    Spec(
        label="Internal Relocations (.data/.bss)",
        payload="test_coff_reloc",
        entry="go",
        expect_stdout="COFF Reloc Test: Global DATA and BSS accessed successfully.",
    ),
    Spec(
        label="Argument Passing",
        payload="test_coff_args",
        entry="go",
        args="test_arguments_string",
        expect_stdout="COFF Args Test: Execution successful.",
    ),
]

def _missing_dir_for(path: str) -> Optional[str]:
    if "build32" in path:
        candidate = BIN32_DIR if "pocs" in path else TEST32_DIR
    elif "build64" in path:
        candidate = BIN64_DIR if "pocs" in path else TEST64_DIR
    else:
        return None
    return candidate if not os.path.isdir(candidate) else None


def preflight_check() -> set[str]:
    missing = set()
    for bin_dir, test_dir, arch in (
        (BIN64_DIR, TEST64_DIR, "x64"),
        (BIN32_DIR, TEST32_DIR, "x86"),
    ):
        missing_bin = not os.path.isdir(bin_dir)
        missing_tests = not os.path.isdir(test_dir)
        if missing_bin:
            missing.add(bin_dir)
        if missing_tests:
            missing.add(test_dir)
        if missing_bin or missing_tests:
            print(
                f"[{Colors.YELLOW}WARN{Colors.RESET}] {arch} build tree incomplete. "
                "Run build.bat tests pocs."
            )
    if missing:
        print()
    return missing


def run_test(tc: TestCase, known_missing: Optional[set[str]] = None) -> Optional[bool]:
    if known_missing:
        if _missing_dir_for(tc.cmd[0]) in known_missing:
            return None
        try:
            payload_index = tc.cmd.index("-f") + 1
            if _missing_dir_for(tc.cmd[payload_index]) in known_missing:
                return None
        except (ValueError, IndexError):
            pass

    print(f"[{Colors.YELLOW}TEST{Colors.RESET}] {tc.name}")

    if not os.path.exists(tc.cmd[0]):
        missing = _missing_dir_for(tc.cmd[0])
        if missing:
            print(f"  {Colors.BLUE}SKIP{Colors.RESET}: Build dependency {missing} not found.")
            return None
        print(f"  {Colors.RED}FAIL{Colors.RESET}: Binary missing: {tc.cmd[0]}")
        return False

    try:
        payload_index = tc.cmd.index("-f") + 1
    except ValueError:
        print(f"  {Colors.RED}FAIL{Colors.RESET}: Test command is missing -f payload argument")
        return False
    if payload_index >= len(tc.cmd) or not os.path.exists(tc.cmd[payload_index]):
        payload = tc.cmd[payload_index] if payload_index < len(tc.cmd) else "<missing>"
        missing = _missing_dir_for(payload)
        if missing:
            print(f"  {Colors.BLUE}SKIP{Colors.RESET}: Build dependency {missing} not found.")
            return None
        print(f"  {Colors.RED}FAIL{Colors.RESET}: Payload missing: {payload}")
        return False

    try:
        if sys.platform == "linux" and os.path.exists("/usr/bin/wine"):
            cmd = ["wine"] + tc.cmd
        else:
            cmd = tc.cmd
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=10,
        )
    except subprocess.TimeoutExpired:
        print(f"  {Colors.RED}FAIL (TIMEOUT){Colors.RESET}")
        return False
    except Exception as exc:
        print(f"  {Colors.RED}FAIL (EXCEPTION){Colors.RESET}: {exc}")
        return False

    stdout, stderr = result.stdout, result.stderr
    found_expected = not tc.expect_stdout or tc.expect_stdout in stdout or tc.expect_stdout in stderr
    expected_code = tc.expect_returncode
    code_ok = result.returncode == (expected_code if expected_code is not None else 0)
    failure_expected = tc.expect_fail
    failed = not code_ok

    if (failure_expected == failed) and found_expected:
        print(f"  {Colors.GREEN}PASS{Colors.RESET}")
        return True

    print(f"  {Colors.RED}FAIL{Colors.RESET}")
    if failure_expected and not failed:
        print("    Reason: expected non-zero exit but process succeeded.")
    elif not failure_expected and failed:
        expected = expected_code if expected_code is not None else 0
        print(f"    Reason: expected exit code {expected}, got {result.returncode}.")
    elif not found_expected:
        print("    Reason: expected output not found in stdout or stderr.")
    print(f"    Expected: '{tc.expect_stdout or '(none)'}'")
    print(f"    Matched : {found_expected}")
    print(f"    --- STDOUT ---\n{stdout}")
    print(f"    --- STDERR ---\n{stderr}")
    return False

def main():
    import argparse

    parser = argparse.ArgumentParser(description="SindriKit COFF Integration Test Runner")
    parser.add_argument(
        "--arch",
        action="append",
        choices=list(ARCHES),
        help="Only run the given architecture (repeatable).",
    )
    parser.add_argument(
        "--exclude-substr",
        action="append",
        default=[],
        metavar="S",
        help="Skip test cases whose name contains S (repeatable).",
    )
    args = parser.parse_args()

    print(f"\n{Colors.BLUE}=== SindriKit COFF Integration Tests ==={Colors.RESET}\n")

    cases = []
    for backend_name, backend_label, backend_args in BACKENDS:
        for spec in SPECS:
            for arch in ARCHES:
                cases.append(spec.to_test_case(backend_label, backend_args, arch))

    if args.arch:
        cases = [t for t in cases if any(f"({a})" in t.name for a in args.arch)]
    if args.exclude_substr:
        cases = [t for t in cases if not any(s in t.name for s in args.exclude_substr)]

    known_missing = preflight_check()
    passed = 0
    skipped = 0

    for idx, tc in enumerate(cases, 1):
        result = run_test(tc, known_missing)
        if result is True:
            passed += 1
        elif result is None:
            skipped += 1

    ran = len(cases) - skipped
    print("==================================================")
    if skipped:
        print(
            f"[{Colors.YELLOW}INFO{Colors.RESET}] {skipped}/{len(cases)} tests skipped "
            "(build tree incomplete. Run build.bat tests pocs)."
        )
    if ran == 0:
        print(f"Result: {Colors.YELLOW}No tests ran.{Colors.RESET}")
        sys.exit(0)

    failed = ran - passed
    pct = (passed / ran) * 100
    color = Colors.GREEN if failed == 0 else Colors.RED
    print(
        f"Result: {color}{pct:.1f}%. {passed}/{ran} passed"
        f"{', ' + str(failed) + ' failed' if failed else ''}"
        f"{f' ({skipped} skipped)' if skipped else ''}{Colors.RESET}"
    )
    sys.exit(0 if failed == 0 else 1)

if __name__ == "__main__":
    main()

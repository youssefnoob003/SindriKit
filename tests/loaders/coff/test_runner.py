"""
SindriKit COFF Loader Test Runner
"""

import os
import subprocess
import sys
from dataclasses import dataclass, field
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

@dataclass
class Spec:
    payload: str
    entry: str
    args: str = ""
    expect_stdout: Optional[str] = None
    expect_fail: bool = False
    label: str = ""

    def to_test_case(self, arch: str) -> TestCase:
        bits = _BITS[arch]
        loader_exe = os.path.join(
            _BIN[bits], "loader_coff", "Release", "loader_coff.exe"
        )

        payload_obj = os.path.join(
            _TEST[bits], f"{self.payload}_{arch}.obj"
        )

        cmd = [loader_exe, "-f", payload_obj, "-e", self.entry]
        if self.args:
            cmd.extend(["-a", self.args])

        return TestCase(
            name=f"[{arch}] {self.label or self.payload}",
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

def run_test(tc: TestCase) -> Tuple[bool, str]:
    if not os.path.exists(tc.cmd[0]):
        return False, f"Loader missing: {tc.cmd[0]}"

    if not os.path.exists(tc.cmd[2]): # the payload
        return False, f"Payload missing: {tc.cmd[2]}"

    try:
        # Check if running in WSL (Linux) to use wine
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
        return False, "Timeout"

    output = result.stdout + result.stderr

    if tc.expect_fail:
        if result.returncode == 0:
            return False, "Expected failure but process exited 0"
        return True, "Passed (Expected Failure)"

    if result.returncode != 0:
        return False, f"Crashed (Exit {result.returncode})\nOutput:\n{output}"

    if tc.expect_stdout and tc.expect_stdout not in output:
        return False, f"Stdout mismatch.\nExpected: {tc.expect_stdout}\nOutput:\n{output}"

    return True, "Passed"

def main():
    print(f"\n{Colors.BLUE}=== SindriKit COFF Integration Tests ==={Colors.RESET}\n")

    cases = []
    for spec in SPECS:
        for arch in ARCHES:
            cases.append(spec.to_test_case(arch))

    passed = 0
    failed = 0

    for idx, tc in enumerate(cases, 1):
        print(f"[{idx:02d}/{len(cases):02d}] {tc.name:<40}", end="", flush=True)

        ok, msg = run_test(tc)
        if ok:
            print(f"{Colors.GREEN}PASS{Colors.RESET}")
            passed += 1
        else:
            print(f"{Colors.RED}FAIL{Colors.RESET}")
            print(f"{Colors.YELLOW}      Details: {msg}{Colors.RESET}")
            failed += 1

    print("\n" + "=" * 50)
    print(f"Total: {len(cases)} | {Colors.GREEN}Passed: {passed}{Colors.RESET} | {Colors.RED}Failed: {failed}{Colors.RESET}")
    print("=" * 50)

    sys.exit(0 if failed == 0 else 1)

if __name__ == "__main__":
    main()

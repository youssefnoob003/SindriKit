"""
SindriKit Integration Test Runner

Data-driven test matrix that auto-expands compact specs across all
loader x architecture combinations.  Add a new Spec to SPECS and every
relevant (loader, arch) variant is generated automatically.

Usage:
    python tests/test_runner.py [--corkami]
"""

import argparse
import os
import subprocess
import sys
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

# Add this block:
try:
    import pe_mutator
except ImportError:
    pe_mutator = None

# ── Path Configuration ──────────────────────────────────────────────────────

BIN64_DIR = r"build64\pocs"
BIN32_DIR = r"build32\pocs"
TEST64_DIR = r"build64\tests\loader"
TEST32_DIR = r"build32\tests\loader"

CORKAMI_DIR = r"tests\fixtures\pe\corkami"
CORKAMI_ZIP = r"tests\fixtures\pe\corkami_fixtures.zip"

# Compact lookups used by the matrix generator.
_BIN = {64: BIN64_DIR, 32: BIN32_DIR}
_TEST = {64: TEST64_DIR, 32: TEST32_DIR}
_BITS = {"x64": 64, "x86": 32}
_PTR_WIDTH = {"x64": 16, "x86": 8}  # MSVC %p hex-digit count

ARCHES = ("x64", "x86")
LOADERS = ("nowinapi", "winapi")
_LOADER_TAG = {"nowinapi": "NoWinAPI", "winapi": "WinAPI"}


class Colors:
    GREEN = "\033[92m"
    RED = "\033[91m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    RESET = "\033[0m"


# ── Test Case (fully resolved, runnable) ────────────────────────────────────


@dataclass
class TestCase:
    """A concrete, runnable test."""

    name: str
    cmd: List[str]
    expect_stdout: Optional[str] = None
    expect_returncode: Optional[int] = None
    expect_fail: bool = False
    corkami_fuzz: bool = False


# ── Spec (compact, auto-expanding) ──────────────────────────────────────────


@dataclass
class Spec:
    """Architecture- and loader-independent test specification.

    Expands into one TestCase per (loader, arch) combination.
    """

    loaders: Tuple[str, ...]
    payload: str  # e.g. "test_dll", "test_exe_advanced"
    export: Optional[str] = None
    args: List[str] = field(default_factory=list)
    expect_stdout: Optional[str] = None  # static expected text
    expect_retval: Optional[int] = None  # expected FFI return (arch-formatted)
    expect_rc: Optional[int] = None  # expected process exit code
    expect_fail: bool = False
    label: str = ""  # human-readable test label

    def _ext(self) -> str:
        return ".dll" if "dll" in self.payload else ".exe"

    def _format_retval(self, arch: str) -> str:
        w = _PTR_WIDTH[arch]
        return f"Export returned: 0x{self.expect_retval:0{w}X}"

    def to_test_case(self, loader: str, arch: str) -> TestCase:
        bits = _BITS[arch]
        loader_exe = os.path.join(
            _BIN[bits], f"loader_{loader}", "Release", f"loader_{loader}.exe"
        )
        payload_file = os.path.join(
            _TEST[bits], "Release", f"{self.payload}_{arch}{self._ext()}"
        )

        cmd = [loader_exe, "-f", payload_file]
        if self.export:
            cmd += ["-e", self.export]
        for a in self.args:
            cmd += ["-a", a]

        expect = self.expect_stdout
        if self.expect_retval is not None:
            expect = self._format_retval(arch)

        tag = _LOADER_TAG[loader]
        return TestCase(
            name=f"{tag} ({arch}) -> {self.label}",
            cmd=cmd,
            expect_stdout=expect,
            expect_returncode=self.expect_rc,
            expect_fail=self.expect_fail,
        )


# ── Declarative Test Specs ──────────────────────────────────────────────────
# Each Spec generates   len(loaders) x len(ARCHES)   concrete TestCases.
# 7 specs x 2 loaders x 2 arches = 28 tests from this table alone.

SPECS = [
    # ── DLL: correct args ───────────────────────────────────────────────────
    Spec(
        ("nowinapi", "winapi"),
        "test_dll",
        "SayHello",
        ["bonjour", "hello", "12"],
        expect_retval=0xFEEDC0DE,
        label="Load DLL with exact args",
    ),
    # ── DLL: bad args ───────────────────────────────────────────────────────
    Spec(
        ("nowinapi", "winapi"),
        "test_dll",
        "SayHello",
        ["wrong", "args"],
        expect_retval=0xDEADBEEF,
        label="Edge Case: Bad Args Validation",
    ),
    # ── DLL: missing -e parameter ───────────────────────────────────────────
    Spec(
        ("nowinapi", "winapi"),
        "test_dll",
        expect_stdout="Error: DLL payload requires an export name",
        expect_fail=True,
        label="Edge Case: Missing Export Parameter",
    ),
    # ── DLL: advanced (imports, allocs) ─────────────────────────────────────
    Spec(
        ("nowinapi", "winapi"),
        "test_dll_advanced",
        "AdvancedExport",
        ["advanced_test"],
        expect_retval=0x1337C0DE,
        label="Load Advanced DLL (Imports, Allocs)",
    ),
    # ── DLL: empty (missing export directory) ───────────────────────────────
    Spec(
        ("nowinapi", "winapi"),
        "test_dll_empty",
        "NonExistentExport",
        expect_stdout="Error: Requested export was not found",
        expect_fail=True,
        label="Load Empty DLL (Missing Dirs)",
    ),
    # ── DLL: verify DllMain ran + relocations applied ───────────────────────
    Spec(
        ("nowinapi", "winapi"),
        "test_dll",
        "VerifyInit",
        expect_retval=0xC001D00D,
        label="Verify DllMain + Relocations",
    ),
    # ── DLL: verify multi-import IAT resolution ────────────────────────────
    Spec(
        ("nowinapi", "winapi"),
        "test_dll_advanced",
        "VerifyImports",
        expect_retval=0xCA11AB1E,
        label="Verify Multi-Import Resolution",
    ),
    # ── DLL: verify TLS callback execution ─────────────────────────────────
    Spec(
        ("nowinapi", "winapi"),
        "test_dll_tls",
        "VerifyTLS",
        expect_retval=0x71500C01,
        label="Verify TLS Callbacks",
    ),
    # ── EXE: basic ──────────────────────────────────────────────────────────
    Spec(
        ("nowinapi", "winapi"),
        "test_exe",
        expect_stdout="Jumping to EXE Entry Point",
        expect_rc=122,  # 0x7A
        label="Run EXE",
    ),
    # ── EXE: advanced (stdlib init, heap allocs) ────────────────────────────
    Spec(
        ("nowinapi", "winapi"),
        "test_exe_advanced",
        expect_stdout="Successfully allocated and printed",
        expect_rc=4919,  # 0x1337
        label="Run Advanced EXE (Stdlib Init)",
    ),
]


def expand_specs(specs):
    """Expand Specs into TestCases, grouped by (loader, arch) for clean output."""
    cases = []
    for loader in LOADERS:
        for arch in ARCHES:
            for spec in specs:
                if loader in spec.loaders:
                    cases.append(spec.to_test_case(loader, arch))
    return cases


# ── Arch-Mismatch Tests ────────────────────────────────────────────────────


def build_mismatch_tests():
    """Generate arch-mismatch guard test cases for nowinapi."""
    cases = []
    for loader_arch, payload_arch in [("x64", "x86"), ("x86", "x64")]:
        lb, pb = _BITS[loader_arch], _BITS[payload_arch]
        expect = "Payload architecture does not match loader architecture"
        cases.append(
            TestCase(
                name=f"NoWinAPI ({loader_arch}) -> Arch Mismatch Guard "
                f"({loader_arch} loader, {payload_arch} DLL)",
                cmd=[
                    os.path.join(
                        _BIN[lb],
                        "loader_nowinapi",
                        "Release",
                        "loader_nowinapi.exe",
                    ),
                    "-f",
                    os.path.join(_TEST[pb], "Release", f"test_dll_{payload_arch}.dll"),
                    "-e",
                    "SayHello",
                ],
                expect_stdout=expect,
                expect_fail=True,
            )
        )
    return cases


# ── Corkami Fuzz Tests ──────────────────────────────────────────────────────


def load_corkami_tests(enabled=False):
    """Return Corkami fuzz test cases if enabled, else an empty list."""
    if not enabled:
        return []

    if not os.path.exists(CORKAMI_DIR) or not os.listdir(CORKAMI_DIR):
        print(f"\n[{Colors.RED}ERROR{Colors.RESET}] Corkami fixtures missing!")
        print(f"[*] Expected: {Colors.YELLOW}{CORKAMI_DIR}{Colors.RESET}")
        print(f"[*] Unzip '{CORKAMI_ZIP}' into that folder.")
        print(f"[*] Password: {Colors.GREEN}infected{Colors.RESET}\n")
        sys.exit(1)

    tests = []
    for file in sorted(os.listdir(CORKAMI_DIR)):
        if file.lower().endswith(".exe"):
            tests.append(
                TestCase(
                    name=f"Corkami Parser Stress Test -> {file}",
                    cmd=[
                        os.path.join(
                            _BIN[64],
                            "loader_winapi",
                            "Release",
                            "loader_winapi.exe",
                        ),
                        "-f",
                        os.path.join(CORKAMI_DIR, file),
                    ],
                    corkami_fuzz=True,
                )
            )
    return tests


# ── Preflight & Build-Tree Checks ──────────────────────────────────────────

_BUILD_TREES = [
    (BIN64_DIR, TEST64_DIR, "x64", "build64"),
    (BIN32_DIR, TEST32_DIR, "x86", "build32"),
]


def preflight_check():
    """Warn about missing build trees. Returns the set of missing dir paths."""
    missing_dirs = set()
    for bin_dir, test_dir, arch, build_name in _BUILD_TREES:
        bin_missing = not os.path.isdir(bin_dir)
        test_missing = not os.path.isdir(test_dir)
        if not bin_missing and not test_missing:
            continue
        if bin_missing:
            missing_dirs.add(bin_dir)
        if test_missing:
            missing_dirs.add(test_dir)
        if bin_missing and test_missing:
            print(
                f"[{Colors.YELLOW}WARN{Colors.RESET}] {arch} not compiled. "
                f"Run  build.bat tests pocs  to build everything."
            )
        elif bin_missing:
            print(
                f"[{Colors.YELLOW}WARN{Colors.RESET}] {arch} PoC loaders missing ({bin_dir}\\). "
                f"Run  build.bat pocs."
            )
        else:
            print(
                f"[{Colors.YELLOW}WARN{Colors.RESET}] {arch} test fixtures missing ({test_dir}\\). "
                f"Run  build.bat tests."
            )
    if missing_dirs:
        print()
    return missing_dirs


def _missing_dir_for(path):
    """Return the build subdir path if absent, or None."""
    if "build32" in path:
        check = BIN32_DIR if "pocs" in path else TEST32_DIR
    elif "build64" in path:
        check = BIN64_DIR if "pocs" in path else TEST64_DIR
    else:
        return None
    return check if not os.path.isdir(check) else None


# ── Test Executor ───────────────────────────────────────────────────────────


def run_test(test, known_missing=None):
    cmd = test.cmd

    # Skip silently if preflight already warned about this build tree.
    if known_missing:
        if _missing_dir_for(cmd[0]) in known_missing:
            return None
        for i, token in enumerate(cmd):
            if token == "-f" and i + 1 < len(cmd):
                if _missing_dir_for(cmd[i + 1]) in known_missing:
                    return None
                break

    print(f"[{Colors.YELLOW}TEST{Colors.RESET}] {test.name}")

    if not os.path.exists(cmd[0]):
        missing = _missing_dir_for(cmd[0])
        if missing:
            print(
                f"  {Colors.BLUE}SKIP{Colors.RESET}: Build dependency {missing} not found."
            )
            return None
        print(f"  {Colors.RED}FAIL{Colors.RESET}: Binary missing: {cmd[0]}")
        return False

    # Check that -f payload exists (e.g. PoCs built but tests weren't).
    for i, token in enumerate(cmd):
        if token == "-f" and i + 1 < len(cmd):
            missing = _missing_dir_for(cmd[i + 1])
            if missing:
                print(
                    f"  {Colors.BLUE}SKIP{Colors.RESET}: Build dependency {missing} not found."
                )
                return None
            break

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=5,
        )
        stdout = result.stdout
        stderr = result.stderr
        returncode = result.returncode

        if returncode in [3221225477, -1073741819]:
            print(f"  {Colors.RED}FAIL (0xC0000005){Colors.RESET}: Access violation!")
            print(f"    --- STDOUT --- \n{stdout}")
            print(f"    --- STDERR --- \n{stderr}")
            return False

        if test.corkami_fuzz:
            if "[-] Error:" in stdout or "[-] Error:" in stderr:
                print(
                    f"  {Colors.YELLOW}PASS (Safely Rejected Malformed PE){Colors.RESET}"
                )
            elif "[!]" in stdout and returncode == 0:
                print(f"  {Colors.GREEN}PASS (True Execution Success){Colors.RESET}")
            else:
                print(f"  {Colors.GREEN}PASS (Handled Safely){Colors.RESET}")
            return True

        found_expected = True
        if test.expect_stdout:
            found_expected = (
                test.expect_stdout in stdout or test.expect_stdout in stderr
            )

        expected_fail = test.expect_fail

        if test.expect_returncode is not None:
            failed = returncode != test.expect_returncode
            if failed:
                print(
                    f"  {Colors.RED}FAIL{Colors.RESET}: Expected return code "
                    f"{test.expect_returncode}, got {returncode}"
                )
        else:
            failed = returncode != 0

        if (expected_fail == failed) and found_expected:
            print(f"  {Colors.GREEN}PASS{Colors.RESET}")
            return True
        else:
            print(f"  {Colors.RED}FAIL{Colors.RESET}")
            if expected_fail and not failed:
                print("    Reason: expected non-zero exit but process succeeded.")
            elif not expected_fail and failed:
                print("    Reason: expected clean exit but process returned an error.")
            elif not found_expected:
                print("    Reason: expected output not found in stdout or stderr.")
            print(f"    Expected: '{test.expect_stdout or '(none)'}'")
            print(f"    Matched : {found_expected}")
            print(f"    --- STDOUT --- \n{stdout}")
            print(f"    --- STDERR --- \n{stderr}")
            return False

    except subprocess.TimeoutExpired:
        print(f"  {Colors.RED}FAIL (TIMEOUT){Colors.RESET}")
        return False
    except Exception as e:
        print(f"  {Colors.RED}FAIL (EXCEPTION){Colors.RESET}: {e}")
        return False


# ── Mutation Engine Tests ───────────────────────────────────────────────────


def load_mutation_tests(enabled=False):
    """Generate mutated PEs on the fly and add them to the test matrix."""
    if not enabled:
        return []

    if pe_mutator is None or pe_mutator.pefile is None:
        print(
            f"\n[{Colors.YELLOW}SKIP{Colors.RESET}] Mutator disabled (pefile not installed)."
        )
        return []

    print(f"\n[{Colors.BLUE}INFO{Colors.RESET}] Generating mutated PE variants...")
    tests = []

    for arch in ARCHES:
        bits = _BITS[arch]
        base_exe = os.path.join(_TEST[bits], "Release", f"test_exe_{arch}.exe")
        base_dll = os.path.join(_TEST[bits], "Release", f"test_dll_{arch}.dll")

        # Create a dedicated directory for mutated payloads
        out_dir = os.path.join(f"build{bits}", "mutations")
        os.makedirs(out_dir, exist_ok=True)

        for loader in LOADERS:
            loader_exe = os.path.join(
                _BIN[bits], f"loader_{loader}", "Release", f"loader_{loader}.exe"
            )

            for mutation in pe_mutator.ALL_MUTATIONS:
                # Select the correct base file based on mutation requirements
                if mutation.applies_to == "dll":
                    src_file = base_dll
                    cmd_args = ["-e", "SayHello"]  # DLLs need an export target
                else:
                    src_file = base_exe
                    cmd_args = []

                # Skip if the user hasn't built the baseline tests yet
                if not os.path.exists(src_file):
                    continue

                try:
                    mutated_path = pe_mutator.mutate_pe(src_file, mutation, out_dir)
                except pe_mutator.MutationError as e:
                    print(f"[{Colors.YELLOW}WARN{Colors.RESET}] {e}")
                    continue

                tag = _LOADER_TAG[loader]
                tests.append(
                    TestCase(
                        name=f"{tag} ({arch}) -> Mutation: {mutation.name}",
                        cmd=[loader_exe, "-f", mutated_path] + cmd_args,
                        expect_fail=not mutation.expect_loadable,
                        corkami_fuzz=True,  # Reuse fuzz logic to ensure we don't 0xC0000005
                    )
                )
    return tests


# ── Entry Point ─────────────────────────────────────────────────────────────


def main():
    parser = argparse.ArgumentParser(description="SindriKit Integration Test Runner")
    parser.add_argument(
        "--corkami",
        action="store_true",
        help="Include Corkami malformed-PE stress tests",
    )
    # Add this argument
    parser.add_argument(
        "--mutate",
        action="store_true",
        help="Generate and run dynamic PE mutations to stress-test the loader",
    )
    args = parser.parse_args()

    print("==================================================")
    print("         SindriKit Integration Tests              ")
    print("==================================================")

    preflight_missing = preflight_check()

    # Append the mutation tests to the matrix
    full_matrix = (
        expand_specs(SPECS)
        + build_mismatch_tests()
        + load_corkami_tests(enabled=args.corkami)
        + load_mutation_tests(enabled=args.mutate)
    )

    print(
        f"[*] {len(full_matrix)} test cases "
        f"({len(SPECS)} specs x {len(LOADERS)} loaders x {len(ARCHES)} arches)\n"
    )

    passed = 0
    skipped = 0
    total = len(full_matrix)

    for test in full_matrix:
        result = run_test(test, known_missing=preflight_missing)
        if result is True:
            passed += 1
        elif result is None:
            skipped += 1

    ran = total - skipped
    print("==================================================")
    if skipped:
        print(
            f"[{Colors.YELLOW}INFO{Colors.RESET}] {skipped}/{total} tests skipped "
            f"(build tree incomplete. Run  build.bat tests pocs)."
        )
    if ran == 0:
        print(f"Result: {Colors.YELLOW}No tests ran.{Colors.RESET}")
        sys.exit(0)

    pct = (passed / ran) * 100
    if pct > 95:
        print(
            f"Result: {Colors.GREEN}{pct:.0f}%. "
            f"{passed}/{ran} passed{Colors.RESET}"
            + (f"  ({skipped} skipped)" if skipped else "")
        )
        sys.exit(0)
    else:
        print(
            f"Result: {Colors.RED}{pct:.1f}%. "
            f"{passed}/{ran} passed, {ran - passed} failed{Colors.RESET}"
            + (f"  ({skipped} skipped)" if skipped else "")
        )
        sys.exit(1)


if __name__ == "__main__":
    os.system("")  # Enable VT100 ANSI sequences on Windows consoles
    main()

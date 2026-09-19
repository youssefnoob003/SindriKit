"""Shared infrastructure for the SindriKit integration test runners.

Owns the pass/fail model, the subprocess plumbing and the result accounting so
that the PE, COFF and injection matrices stay declarative and behave
identically. Pure decision logic (evaluate_test) is unit-tested on any host via
selftest_common.py; the process plumbing lives here and is exercised on Windows.
"""

import os
import subprocess
import sys
from dataclasses import dataclass
from typing import Callable, List, Optional, Sequence, Tuple

class Colors:
    GREEN = "\033[92m"
    RED = "\033[91m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    RESET = "\033[0m"

# Exit codes Windows returns for STATUS_ACCESS_VIOLATION.
EXIT_ACCESS_VIOLATION = 3221225477
EXIT_ACCESS_VIOLATION_ALT = -1073741819
CRASH_CODES = {EXIT_ACCESS_VIOLATION, EXIT_ACCESS_VIOLATION_ALT}

# Marker `unified` emits through snd_status_print when a load fails.
REJECT_MARKER = "[ERR]"


def normalize_output(value) -> str:
    """Return subprocess output as text regardless of Python/platform mode."""
    if value is None:
        return ""
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return str(value)


@dataclass
class TestCase:
    name: str
    cmd: List[str]
    expect_output: Optional[str] = None      # substring that must appear (stdout or stderr)
    expect_returncode: Optional[int] = None  # exact expected exit code
    expect_fail: bool = False                # process must exit non-zero
    expect_reject: Optional[str] = None      # e.g. "[ERR]": hostile input must be rejected
    setup_error: Optional[str] = None        # case could not be generated/prepared
    corkami_fuzz: bool = False               # local-only fuzz semantics, never enforced in CI


def evaluate_test(
    tc: TestCase,
    stdout: Optional[str],
    stderr: Optional[str],
    returncode: int,
) -> Tuple[bool, Optional[str], Optional[str]]:
    """Pure pass/fail decision for a TestCase. Returns (passed, reason, pass_message)."""
    out = normalize_output(stdout)
    err = normalize_output(stderr)
    combined = out + "\n" + err

    if returncode in CRASH_CODES:
        return False, f"process crashed (access violation, rc={returncode})", None

    if tc.corkami_fuzz:
        if "[-] Error:" in out or "[-] Error:" in err or REJECT_MARKER in combined:
            return True, None, "Safely Rejected Malformed PE"
        elif "[!]" in out and returncode == 0:
            return True, None, "Executed Successfully"
        else:
            return True, None, "Did not crash"

    if tc.expect_reject is not None:
        if tc.expect_reject not in combined:
            return False, f"expected rejection marker {tc.expect_reject!r} in the output", None
        if returncode == 0:
            return False, "expected a non-zero exit, got 0 (hostile input was accepted?)", None
        return True, None, None

    if tc.expect_returncode is not None:
        if returncode != tc.expect_returncode:
            return False, f"expected return code {tc.expect_returncode}, got {returncode}", None
    else:
        succeeded = (returncode == 0)
        if succeeded != (not tc.expect_fail):
            if tc.expect_fail:
                return False, "expected a non-zero exit but the process succeeded", None
            return False, f"expected a clean exit but the process returned {returncode}", None

    if tc.expect_output is not None and tc.expect_output not in combined:
        return False, f"expected output {tc.expect_output!r} not found in stdout/stderr", None

    return True, None, None


# ── Build-tree helpers ───────────────────────────────────────────────────────


@dataclass(frozen=True)
class BuildTree:
    arch: str
    bin_dir: str  # directory holding the unified.exe PoC
    test_dir: str  # directory holding the built fixtures


def preflight(trees: Sequence[BuildTree], selected_arches: Optional[Sequence[str]] = None) -> set:
    """Report which build-tree directories are missing for the selected arches."""
    wanted = set(selected_arches or (t.arch for t in trees))
    missing = set()
    for tree in trees:
        if tree.arch not in wanted:
            continue
        for label, d in (("PoC loaders", tree.bin_dir), ("test fixtures", tree.test_dir)):
            if not os.path.isdir(d):
                missing.add(d)
                print(
                    f"[{Colors.YELLOW}WARN{Colors.RESET}] {tree.arch} {label} missing ({d}). "
                    "Run 'build.bat tests pocs' to build everything."
                )
    if missing:
        print()
    return missing


def missing_dir_for(path: str, trees: Sequence[BuildTree]) -> Optional[str]:
    """Return the build-tree dir that is missing and owns `path`, else None."""
    abspath = os.path.abspath(path)
    for tree in trees:
        for rel in (tree.bin_dir, tree.test_dir):
            root = os.path.abspath(rel)
            if abspath == root or abspath.startswith(root + os.sep):
                return rel if not os.path.isdir(rel) else None
    return None


# ── Test executor ────────────────────────────────────────────────────────────


def run_case(
    tc: TestCase,
    trees: Sequence[BuildTree],
    known_missing: Optional[set] = None,
    timeout: float = 10.0,
    pre_run: Optional[Callable[[], Optional[str]]] = None,
    verify: Optional[Callable[[], Tuple[bool, Optional[str]]]] = None,
    teardown: Optional[Callable[[], None]] = None,
    env: Optional[dict] = None,
) -> Optional[bool]:
    """Run one TestCase. Returns True (pass), False (fail), None (skip).

    `pre_run` runs before the subprocess (may mutate tc.cmd); an error string
    returned there fails the case. `verify` runs only after a successful exit
    (e.g. polling for an injected payload's side effect). `teardown` always runs.
    """
    cmd = tc.cmd

    if tc.setup_error is not None:
        print(f"[{Colors.YELLOW}TEST{Colors.RESET}] {tc.name}")
        print(f"  {Colors.RED}FAIL{Colors.RESET}: {tc.setup_error}")
        return False

    if known_missing:
        if missing_dir_for(cmd[0], trees) in known_missing:
            return None
        for i, token in enumerate(cmd):
            if token == "-f" and i + 1 < len(cmd):
                if missing_dir_for(cmd[i + 1], trees) in known_missing:
                    return None
                break

    print(f"[{Colors.YELLOW}TEST{Colors.RESET}] {tc.name}")

    try:
        if pre_run is not None:
            error = pre_run()
            if error is not None:
                print(f"  {Colors.RED}FAIL{Colors.RESET}: {error}")
                return False

        # pre_run may rewrite tc.cmd (e.g. substituting a spawned target PID),
        # so re-read it before building the command line.
        cmd = tc.cmd

        if not os.path.exists(cmd[0]):
            missing = missing_dir_for(cmd[0], trees)
            if missing:
                print(f"  {Colors.BLUE}SKIP{Colors.RESET}: Build dependency {missing} not found.")
                return None
            print(f"  {Colors.RED}FAIL{Colors.RESET}: Binary missing: {cmd[0]}")
            return False

        for i, token in enumerate(cmd):
            if token == "-f" and i + 1 < len(cmd):
                missing = missing_dir_for(cmd[i + 1], trees)
                if missing:
                    print(f"  {Colors.BLUE}SKIP{Colors.RESET}: Build dependency {missing} not found.")
                    return None
                break

        # Use Popen instead of subprocess.run so that teardown (which kills
        # grandchild processes like the injection target) runs BEFORE we
        # drain the pipes.  subprocess.run's internal timeout handling calls
        # process.communicate() without a timeout after kill(), which blocks
        # forever when a grandchild inherits the pipe handles.
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            env=env,
        )
        timed_out = False
        try:
            stdout, stderr = proc.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            proc.kill()
            timed_out = True
            # teardown kills grandchild processes (e.g. the injection target)
            # so that the pipes close and communicate() can drain them.
            if teardown is not None:
                teardown()
                teardown = None  # prevent double-call in finally
            stdout, stderr = proc.communicate(timeout=5)

        if timed_out:
            print(f"  {Colors.RED}FAIL (TIMEOUT){Colors.RESET}")
            return False

        result = subprocess.CompletedProcess(cmd, proc.returncode, stdout, stderr)
        passed, reason, pass_message = evaluate_test(tc, result.stdout, result.stderr, result.returncode)

        if passed and verify is not None:
            passed, reason = verify()
            pass_message = None

        if passed:
            if pass_message:
                print(f"  {Colors.GREEN}PASS ({pass_message}){Colors.RESET}")
            else:
                print(f"  {Colors.GREEN}PASS{Colors.RESET}")
            return True

        print(f"  {Colors.RED}FAIL{Colors.RESET}: {reason}")
        print(f"    Expected: '{tc.expect_output or '(none)'}'")
        print(f"    --- STDOUT --- \n{normalize_output(result.stdout)}")
        print(f"    --- STDERR --- \n{normalize_output(result.stderr)}")
        return False

    except OSError as exc:
        print(f"  {Colors.RED}FAIL (LAUNCH){Colors.RESET}: {exc}")
        return False
    finally:
        if teardown is not None:
            teardown()


# ── Matrix helpers ──────────────────────────────────────────────────────────


def filter_cases(cases: List[TestCase], arches: Optional[Sequence[str]] = None, excluded: Sequence[str] = ()) -> List[TestCase]:
    out = cases
    if arches:
        out = [c for c in out if any(f"({a})" in c.name for a in arches)]
    if excluded:
        out = [c for c in out if not any(s in c.name for s in excluded)]
    return out


def summarize(passed: int, skipped: int, total: int, strict: bool) -> int:
    """Print the run summary. Returns the process exit code."""
    ran = total - skipped
    print("=" * 50)
    if skipped:
        print(
            f"[{Colors.YELLOW}INFO{Colors.RESET}] {skipped}/{total} tests skipped "
            f"(build tree incomplete. Run 'build.bat tests pocs')."
        )
    if ran == 0:
        print(
            f"Result: {Colors.RED}No tests ran ({skipped} skipped).{Colors.RESET} "
            "A green run must contain real test cases."
        )
        return 1

    failed = total - passed if strict else ran - passed
    denominator = total if strict else ran
    pct = (passed / denominator) * 100 if denominator else 0.0
    color = Colors.GREEN if failed == 0 else Colors.RED
    print(
        f"Result: {color}{pct:.1f}%. {passed}/{ran} passed"
        f"{', ' + str(failed) + ' failed' if failed else ''}"
        f"{f' ({skipped} skipped)' if skipped else ''}{Colors.RESET}"
    )
    return 0 if failed == 0 else 1


def vt100_enable() -> None:
    """Enable ANSI escape sequences in Windows console output."""
    if os.name == "nt":
        os.system("")  # noqa: S605


def sys_exit(code: int) -> None:
    sys.exit(code)

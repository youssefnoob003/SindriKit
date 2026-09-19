#!/usr/bin/env python3
"""Host-side self-tests for the integration runner infrastructure.

These run on any platform with a Python 3 interpreter and no Windows build
tree. They guard the pass/fail decision engine and the pure runner helpers in
tests/loaders/runner_core.py, so a regression in the harness itself is caught
without needing unified.exe.
"""

import os
import tempfile
import unittest

sys_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
import sys  # noqa: E402

sys.path.insert(0, os.path.join(sys_path, "loaders"))

from runner_core import (BuildTree, REJECT_MARKER, TestCase, evaluate_test,  # noqa: E402
                         filter_cases, missing_dir_for, preflight, summarize)


def tc(**kw):
    kw.setdefault("name", "test")
    kw.setdefault("cmd", ["unified", "load", "pe", "-f", "payload"])
    return TestCase(**kw)


class EvaluateCleanExit(unittest.TestCase):
    def test_no_expectations_passes_on_zero(self):
        ok, reason = evaluate_test(tc(), "stdout", "stderr", 0)
        self.assertTrue(ok, reason)

    def test_no_expectations_fails_on_nonzero(self):
        ok, reason = evaluate_test(tc(), "stdout", "stderr", 3)
        self.assertFalse(ok, reason)


class EvaluateOutput(unittest.TestCase):
    def test_found(self):
        ok, reason = evaluate_test(tc(expect_output="all good"), "all good\n", "", 0)
        self.assertTrue(ok, reason)

    def test_missing(self):
        ok, reason = evaluate_test(tc(expect_output="all good"), "other\n", "", 0)
        self.assertFalse(ok)
        self.assertIn("all good", reason)

    def test_checked_across_stderr(self):
        ok, reason = evaluate_test(tc(expect_output="msg"), "", "msg\n", 0)
        self.assertTrue(ok, reason)

    def test_no_cross_stream_concat_false_positive(self):
        # stdout "foo" + separator + stderr "bar" must not match "foobar"
        ok, reason = evaluate_test(tc(expect_output="foobar"), "foo", "bar", 0)
        self.assertFalse(ok)


class EvaluateReturncode(unittest.TestCase):
    def test_exact_match(self):
        ok, reason = evaluate_test(tc(expect_returncode=122), "", "", 122)
        self.assertTrue(ok, reason)

    def test_mismatch(self):
        ok, reason = evaluate_test(tc(expect_returncode=122), "", "", 0)
        self.assertFalse(ok)


class EvaluateExpectedFail(unittest.TestCase):
    def test_fail_matches_nonzero(self):
        ok, reason = evaluate_test(tc(expect_fail=True), "", "", 1)
        self.assertTrue(ok, reason)

    def test_fail_does_not_match_zero(self):
        ok, reason = evaluate_test(tc(expect_fail=True), "", "", 0)
        self.assertFalse(ok)


class EvaluateReject(unittest.TestCase):
    def test_rejection_marker_with_nonzero_passes(self):
        ok, reason = evaluate_test(tc(expect_reject=REJECT_MARKER), "", f"{REJECT_MARKER} bad image\n", 1)
        self.assertTrue(ok, reason)

    def test_marker_missing_fails(self):
        ok, reason = evaluate_test(tc(expect_reject=REJECT_MARKER), "", "no marker\n", 1)
        self.assertFalse(ok)
        self.assertIn("rejection marker", reason)

    def test_marker_with_zero_exit_is_a_failure(self):
        ok, reason = evaluate_test(tc(expect_reject=REJECT_MARKER), f"{REJECT_MARKER} ??", "", 0)
        self.assertFalse(ok)

    def test_crash_is_not_an_acceptable_rejection(self):
        ok, reason = evaluate_test(tc(expect_reject=REJECT_MARKER), "", "", 3221225477)
        self.assertFalse(ok)


class EvaluateFuzz(unittest.TestCase):
    def test_crash_fails(self):
        ok, reason = evaluate_test(tc(corkami_fuzz=True), "", "", 3221225477)
        self.assertFalse(ok)

    def test_non_crash_passes(self):
        ok, reason = evaluate_test(tc(corkami_fuzz=True), "", "no output", 0)
        self.assertTrue(ok, reason)
        ok, reason = evaluate_test(tc(corkami_fuzz=True), "", f"{REJECT_MARKER} rejected", 1)
        self.assertTrue(ok, reason)


class FilterCases(unittest.TestCase):
    def _cases(self):
        return [
            TestCase(name="Win32 (x64) -> A", cmd=[]),
            TestCase(name="Win32 (x86) -> B", cmd=[]),
            TestCase(name="Syscalls (x64) -> C", cmd=[]),
        ]

    def test_arch_filter(self):
        out = filter_cases(self._cases(), arches=("x86",))
        self.assertEqual([c.name for c in out], ["Win32 (x86) -> B"])

    def test_exclude_substr(self):
        out = filter_cases(self._cases(), excluded=("Syscalls",))
        self.assertEqual(len(out), 2)

    def test_no_arch_keeps_all(self):
        out = filter_cases(self._cases())
        self.assertEqual(len(out), 3)


class Summarize(unittest.TestCase):
    def test_green_non_strict(self):
        self.assertEqual(summarize(10, 0, 10, strict=False), 0)

    def test_fail_non_strict(self):
        self.assertEqual(summarize(9, 0, 10, strict=False), 1)

    def test_strict_counts_skips_as_failures(self):
        self.assertEqual(summarize(8, 2, 10, strict=True), 1)

    def test_strict_green(self):
        self.assertEqual(summarize(10, 0, 10, strict=True), 0)

    def test_no_tests_ran(self):
        self.assertEqual(summarize(0, 2, 2, strict=False), 1)


class MissingDirFor(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        root = self._tmp.name
        self.bin = os.path.join(root, "build", "bin")
        self.test = os.path.join(root, "build", "test")
        os.makedirs(self.bin)
        self.trees = (BuildTree("x64", self.bin, self.test),)

    def tearDown(self):
        self._tmp.cleanup()

    def test_under_missing_test_dir_returns_it(self):
        # bin exists, test dir missing
        self.assertEqual(missing_dir_for(os.path.join(self.test, "fixture"), self.trees), self.test)

    def test_under_present_bin_returns_none(self):
        self.assertIsNone(missing_dir_for(os.path.join(self.bin, "unified.exe"), self.trees))

    def test_under_test_dir_once_created_returns_none(self):
        os.makedirs(self.test)
        self.assertIsNone(missing_dir_for(os.path.join(self.test, "fixture"), self.trees))

    def test_unrelated_path_returns_none(self):
        self.assertIsNone(missing_dir_for("/elsewhere/payload.bin", self.trees))


class Preflight(unittest.TestCase):
    def test_reports_only_selected_arch(self):
        with tempfile.TemporaryDirectory() as root:
            trees = (
                BuildTree("x64", os.path.join(root, "a"), os.path.join(root, "b")),
                BuildTree("x86", os.path.join(root, "c"), os.path.join(root, "d")),
            )
            missing = preflight(trees, selected_arches=("x64",))
            self.assertEqual(missing, {os.path.join(root, "a"), os.path.join(root, "b")})


if __name__ == "__main__":
    unittest.main(verbosity=2)
# Loader Test Suite

This directory contains the full integration test infrastructure for the reflective loading pipeline. It validates that both the Win32 (`loader_winapi`) and Native (`loader_nowinapi`) loaders correctly handle DLLs, EXEs, edge cases, and structurally corrupted PE inputs across both x64 and x86 architectures.

## Table of Contents
- [test_runner.py](test_runner.py)
  The data-driven integration test runner. Expands compact `Spec` definitions into concrete `TestCase` objects across all `(loader × architecture)` combinations, executes them, and validates stdout output, FFI return values, and process exit codes.
- [pe_mutator.py](pe_mutator.py)
  PE mutation engine for stress-testing. Applies targeted structural mutations to valid PE files to produce variants that exercise parser bounds checking and error handling.
- [CMakeLists.txt](CMakeLists.txt)
  Build definitions for the test payload binaries. Compiles each test source into an architecture-suffixed output (e.g., `test_dll_x64.dll`).
- [src/](src/)
  Source code for the test payload binaries loaded by the runner.

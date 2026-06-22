# Integration Tests

This directory contains the automated testing infrastructure for validating the SindriKit reflective loading pipeline. Tests are data-driven: a compact specification matrix auto-expands across all `(loader, architecture)` combinations, and a PE mutation engine stress-tests the parser against structurally malformed inputs.

## Table of Contents
- [loader/](loader/)
  The reflective loader test suite, test runner, PE mutator, build definitions, and payload source code.
- [fixtures/](fixtures/)
  Pre-built binary test assets (e.g., Corkami PE corpus) used for parser fuzz testing.

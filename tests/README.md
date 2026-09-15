# Integration Tests

This directory contains the automated testing infrastructure for validating the SindriKit PE and COFF loading pipelines. Tests are data-driven: compact specification matrices expand across backend and architecture combinations, and a PE mutation engine stress-tests the parser against structurally malformed inputs.

## Table of Contents
- [loader/](loader/)
  The pe loader test suite, test runner, PE mutator, build definitions, and payload source code.
- [fixtures/](fixtures/)
  Pre-built binary test assets (e.g., Corkami PE corpus) used for parser fuzz testing.

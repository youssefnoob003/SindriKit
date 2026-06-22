# Syscall Primitives

This directory details SindriKit's advanced system for extracting System Service Numbers (SSNs) and bypassing inline userland EDR hooks.

> [!WARNING]
> **Initialization Constraint:** The syscall pipeline requires setting up the NTDLL base address. `snd_set_ntdll()` must be called prior to invoking any resolution function.

## Table of Contents
- [pipeline.md](pipeline.md)
  Explains the cascading fallback mechanism used to evaluate multiple SSN resolution strategies sequentially.
- [gates.md](gates.md)
  Provides technical breakdowns of the Hell's, Halo's, Tartarus', and VelesReek extraction techniques.
- [api_reference.md](api_reference.md)
  Public API documentation for syscall resolvers, pipeline configuration, and the generic ASM invocation stub.

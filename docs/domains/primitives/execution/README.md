# Execution Primitives

This directory contains techniques and API references for executing arbitrary memory pointers and transitioning across OS bitness boundaries.

## Table of Contents
- [ffi.md](ffi.md)
  Details the MASM assembly bridges used to invoke dynamically resolved function pointers.
- [heavens_gate.md](heavens_gate.md)
  Explains the transition from a 32-bit WoW64 process into native 64-bit mode using the `0x33` segment selector.
- [api_reference.md](api_reference.md)
  Public API documentation for the dynamic execution and Heaven's Gate functions.

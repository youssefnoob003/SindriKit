# Memory Primitives

This directory documents the paradigms used by the framework to allocate, protect, and free virtual memory within the host process.

> [!CAUTION]
> **OpSec Warning:** Selecting the `snd_mem_win` paradigm actively routes memory allocations through userland EDR hooks. Operators must use `snd_mem_native` for evasive deployments.

## Table of Contents
- [techniques.md](techniques.md)
  Contrasts the telemetry differences between standard Win32 allocation (`VirtualAlloc`) and Native direct syscalls.
- [api_reference.md](api_reference.md)
  Public API documentation detailing the `snd_memory_api_t` interface and its pre-built instances.

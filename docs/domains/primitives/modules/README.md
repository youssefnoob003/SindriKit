# Module Primitives

This directory covers techniques for resolving loaded module base addresses and extracting exported functions without relying on highly-monitored OS APIs.

## Table of Contents
- [peb_walking.md](peb_walking.md)
  Explains how to manually traverse the `PEB->Ldr` lists to locate DLLs in memory.
- [api_reference.md](api_reference.md)
  Public API documentation for PEB walking functions and the `snd_module_api_t` interface.

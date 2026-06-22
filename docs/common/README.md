# Common Infrastructure

This directory documents the shared, domain-agnostic utilities that underpin the entire SindriKit framework. These headers live under `include/sindri/common/` and provide CRT-independent primitives, buffer lifecycle management, API hashing, and the diagnostic status system.

## Table of Contents
- [infrastructure.md](infrastructure.md)
  Conceptual breakdown of how SindriKit achieves full CRT independence, how buffer bounds tracking prevents memory corruption, and how the compile-time hashing pipeline eliminates plaintext strings.
- [api_reference.md](api_reference.md)
  Complete API documentation for buffers, hashing, CRT replacement primitives, disk I/O, and the debug output system.

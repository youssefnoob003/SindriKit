# Parsers Domain

This directory contains strict parsing engines for interpreting complex file formats (like Portable Executables) purely in-memory. 

## Table of Contents
- [techniques.md](techniques.md)
  Covers the PE format, the `is_mapped` flag, bounds checking philosophy, export/import resolution mechanics, and base relocation patching.
- [api_reference.md](api_reference.md)
  Complete API documentation for `snd_pe_parse`, export/import resolution, relocations, TLS callbacks, and utility macros.

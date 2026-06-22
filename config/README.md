# Build Configuration

This directory holds declarative configuration files consumed by the build system's pre-build scripts. These files define *what* the framework needs to hash, and the scripts in `scripts/` determine *how*.

## Table of Contents
- [hashes.ini](hashes.ini)
  The single source of truth for all compile-time API string hashes. Organized into `[module::<dll_name>]` sections that map DLL names to their exported functions. The build system generates a hash for both the module name and every API listed beneath it.

# Build Scripts

This directory contains pre-build automation scripts invoked by the CMake build system. These scripts are not executed at runtime, they generate source code artifacts consumed during compilation.

## Table of Contents
- [generate_hashes.py](generate_hashes.py)
  Python script that reads the hash manifest (`config/hashes.ini`) and emits a C header of pre-computed compile-time hash macros. Generates a randomized 32-bit seed on every invocation to defeat static analysis rainbow tables.

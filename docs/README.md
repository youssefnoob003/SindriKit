# SindriKit Documentation

This is the root of the SindriKit technical documentation tree. It is organized to mirror the framework's internal architecture: shared infrastructure at the base, architectural patterns above it, domain-specific techniques and API references at the leaves.

## Table of Contents

### 1. Introduction & Onboarding
- [**getting_started/**](getting_started/)
  Onboarding guides for building SindriKit, configuring CMake, and running the first reflective loader.
- [**examples/**](examples/)
  Standalone PoC walkthroughs demonstrating the framework across three operational profiles.

### 2. The Core Framework
- [**architecture/**](architecture/)
  Core design patterns governing the framework: Dependency Injection, State Machines, the Status System, and Red Team integration guidance.
- [**domains/**](domains/)
  The actionable offensive capabilities: primitives, loaders, and planned future domains (evasion, injection).

### 3. Internal Engines & Utilities
- [**parsers/**](parsers/)
  PE format parsing engines: header validation, export/import resolution, base relocations, and TLS callback execution.
- [**common/**](common/)
  Shared, domain-agnostic utilities: CRT-independent primitives, buffer bounds tracking, API hashing, disk I/O, and the debug output system.

### 4. Build System & Quality Assurance
- [**config/**](config/)
  Documentation for the declarative configuration files (hash manifests) consumed by the build system.
- [**scripts/**](scripts/)
  Documentation for the pre-build automation scripts that generate compile-time hash headers.
- [**tests/**](tests/)
  Documentation for the integration testing infrastructure: the data-driven test runner, PE mutation engine, and test payload fixtures.

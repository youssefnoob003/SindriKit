# Core Architecture

This directory outlines the fundamental design patterns, execution models, and telemetry constraints governing the entire SindriKit framework.

> [!CAUTION]
> **No Direct OS API Calls:** Domains in this framework are strictly forbidden from making direct calls to OS APIs (like `VirtualAlloc` or `NtProtectVirtualMemory`). All interactions must occur through Dependency Injection tables.

## Table of Contents
- [dependency_injection.md](dependency_injection.md)
  Explanation of the `snd_memory_api_t` and `snd_module_api_t` interface contracts that decouple offensive intent from execution mechanics.
- [state_machines.md](state_machines.md)
  Details how complex operations (like loading or injecting) are broken into discrete, pausable stages for robust error handling.
- [status_system.md](status_system.md)
  Documents the `snd_status_t` structure and the `SND_ENABLE_DEBUG` preprocessor macro used to strip plaintext error telemetry.
- [redteam_integration.md](redteam_integration.md)
  Guide on linking SindriKit as an embedded engine into larger C2 implants and staging pipelines.

# Evasion Techniques

> [!NOTE]
> The evasion domain is planned for a future release of SindriKit. This page will document defensive evasion primitives as they are implemented.

## Overview

The evasion domain will expose standalone, context-driven offensive evasion capabilities that follow the same State Machine and Dependency Injection patterns used throughout the toolkit.

Planned capabilities include:

- **ETW Patching** — Disabling Event Tracing for Windows telemetry at the userland level by patching `EtwEventWrite` within the process's own ntdll copy.
- **AMSI Bypass** — Neutralizing the Antimalware Scan Interface within a process to prevent in-memory script and buffer scanning.
- **Call Stack Spoofing** — Forging the userland call stack prior to making sensitive API calls (e.g., memory allocation) to avoid stack-based heuristics used by EDRs.
- **Sleep Obfuscation** — Encrypting the implant's memory regions while sleeping to evade periodic memory scanner heuristics.

Each evasion capability will be initialized via a dedicated context struct and will expose its execution mechanics through the standard injected `mem_api` / `mod_api` DI tables.

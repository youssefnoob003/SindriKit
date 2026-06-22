# Injection Techniques

> [!NOTE]
> The injection domain is planned for a future release of SindriKit. This page will document process injection capabilities as they are implemented.

## Overview

The injection domain will expose process injection capabilities that follow the same State Machine and Dependency Injection patterns used throughout the toolkit. A `snd_injector_ctx_t` context will manage the progression of injection operations through discrete, trackable stages.

Planned capabilities include:

- **Classic Remote Injection** — Standard `VirtualAllocEx` / `WriteProcessMemory` / `CreateRemoteThread` injection, exposed through the injected `mem_api` so the allocation step can be transparently replaced with a direct `NtAllocateVirtualMemory` syscall.
- **Process Hollowing** — Unmapping a legitimate process's image and replacing it with a payload image, applying relocations and resolving imports in the remote process context.
- **Thread Hijacking** — Suspending a target thread, overwriting its context to redirect execution to a shellcode or reflectively-loaded payload, then resuming.
- **APC Queue Injection** — Queuing an Asynchronous Procedure Call to an alertable thread in the target process using `NtQueueApcThread`.

Each technique will rely on the existing PE parser and reflective loading pipeline from the loaders domain, reusing all established primitives rather than re-implementing them.

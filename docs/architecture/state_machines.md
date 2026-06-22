# State Machines

SindriKit is fundamentally designed around the concept of discrete State Machines. This is a global toolkit architecture pattern used to track the progression of any offensive capability—whether that is reflective loading, remote process injection, call stack spoofing, or token manipulation.

Instead of executing complex offensive operations as a single, monolithic function, tasks are broken down into logical, sequential stages managed by a persistent context structure.

## The Global Context Pattern

Every major domain within SindriKit revolves around a domain-specific context structure. 

While the exact fields vary depending on the operation, every context struct adheres to a common pattern:
1. **Execution Primitives**: Pointers to the injected OS API tables (e.g., `mem_api`, `mod_api`) that govern how the operation interacts with the host.
2. **Target Data**: Pointers, handles, and metadata regarding the objective of the operation.
3. **The State Tracker**: An enumeration (`stage`) tracking precisely how far the operation has progressed.

## Advantages of State Machines

By decentralizing execution into trackable stages, the framework gains several critical capabilities:

### 1. Granular Error Handling
If an operation fails, the context retains the exact stage where the failure occurred. This allows operators to pinpoint whether a payload failed during memory allocation, import resolution, or thread execution, enabling rapid triage without needing verbose debug logs that could compromise OpSec.

### 2. Execution Pausing and Sleep Obfuscation
Because the state of the operation is entirely self-contained within the context structure, execution can be intentionally suspended. An operator can allocate memory, put the implant to sleep for a significant duration (to evade heuristic timing checks and memory scanners), and then safely resume mapping sections at a later time. The state machine ensures the engine picks up exactly where it left off.

### 3. Enforced Ordering
The framework's internal API functions validate the current `stage` before executing. This prevents undefined behavior or out-of-order execution during manual, staged, or highly fragmented deployments.

## Case Study: The Reflective Loader

To demonstrate how this global pattern is implemented, the Reflective Loader domain—the first concrete system built into SindriKit—serves as the primary reference.

The loader utilizes the `snd_loader_ctx_t` structure (defined in `include/sindri/loaders/reflective/engine.h`), which tracks its progression using the `snd_loader_stage_t` enumeration:

- `SND_STAGE_UNINITIALIZED`: The context has been created but no PE parsing has occurred.
- `SND_STAGE_PARSED`: The raw payload has been validated and parsed.
- `SND_STAGE_MEM_ALLOCATED`: Virtual memory has been allocated via the injected `mem_api`.
- `SND_STAGE_SECTIONS_MAPPED`: PE sections have been copied to the virtual base.
- `SND_STAGE_RELOCATED`: Base relocations have been applied.
- `SND_STAGE_IMPORTS_RESOLVED`: The IAT has been populated via the injected `mod_api`.
- `SND_STAGE_READY_FOR_EXECUTION`: Memory protections have been applied; the payload is ready.
- `SND_STAGE_EXECUTED`: The entry point has been invoked.

For example, if an operator attempts to call `snd_resolve_imports(ctx)`, the engine will explicitly verify that the context has at least reached `SND_STAGE_SECTIONS_MAPPED`. If it has not, the engine aborts and returns an error (`SND_STATUS_INVALID_STAGE_SEQUENCE`), preserving memory integrity and preventing catastrophic crashes.

As future domains (such as injectors and spoofers) are added to the toolkit, they will inherently inherit and implement this identical State Machine paradigm.

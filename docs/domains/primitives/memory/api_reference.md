# Memory: API Reference

This page documents the local memory capabilities exported by the framework. Type definitions live in `include/sindri/primitives/os_api.h`; pre-built instances are declared in `include/sindri/primitives/memory.h`.

---

## Memory API Instances (`sindri/primitives/memory.h`)

Three pre-built instances of the `snd_memory_api_t` interface are exported globally. These pointers are typically assigned to `ctx.mem_api` during loader or injection initialization.

| Symbol | Backend | Source |
|---|---|---|
| `snd_mem_win` | Win32 API (`VirtualAlloc`, `VirtualProtect`, `VirtualFree`) | `src/primitives/memory/win.c` |
| `snd_mem_nt` | NT API via PEB + EAT resolution (`NtAllocateVirtualMemory`, etc.) | `src/primitives/memory/nt.c` |
| `snd_mem_sys` | Direct syscalls via SSN resolution + ASM stub | `src/primitives/memory/sys.c` |

---

## The Interface (`sindri/primitives/os_api.h`)

### `snd_memory_api_t`

Function pointer table defining the local memory contract. Operators can implement custom instances to route operations through alternative mechanisms (hypervisors, vulnerable drivers, etc.).

| Field | Signature | Description |
|---|---|---|
| `alloc` | `snd_status_t (*)(LPVOID address, SIZE_T size, DWORD alloc_type, DWORD protect, LPVOID *out_address)` | Allocates virtual memory in the current process |
| `free` | `snd_status_t (*)(LPVOID address, SIZE_T size, DWORD free_type)` | Frees or decommits virtual memory |
| `protect` | `snd_status_t (*)(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect)` | Changes page protections |

#### `alloc`

| Parameter | Description |
|---|---|
| `address` | Preferred base address, or `NULL` to let the OS choose |
| `size` | Number of bytes to allocate |
| `alloc_type` | Win32 allocation flags (`MEM_COMMIT`, `MEM_RESERVE`, etc.) |
| `protect` | Initial page protection (`PAGE_READWRITE`, `PAGE_EXECUTE_READ`, etc.) |
| `out_address` | Receives the allocated base address on success |

**Status codes:** `SND_STATUS_ALLOC_FAILED`, `SND_STATUS_INVALID_PARAMETER`

#### `free`

| Parameter | Description |
|---|---|
| `address` | Base address of the region to free |
| `size` | Size of the region (`0` when using `MEM_RELEASE`) |
| `free_type` | Win32 free flags (`MEM_RELEASE`, `MEM_DECOMMIT`) |

The NT and syscall backends treat a zero `size` on free as a no-op success.

**Status codes:** `SND_STATUS_VIRTUAL_FREE_FAILED`, `SND_STATUS_INVALID_PARAMETER`

#### `protect`

| Parameter | Description |
|---|---|
| `address` | Base address of the region |
| `size` | Size of the region to reprotect |
| `new_protect` | Desired page protection |
| `old_protect` | Optional; receives the previous protection on success |

**Status codes:** `SND_STATUS_PROTECTION_UPDATE_FAILED`, `SND_STATUS_INVALID_PARAMETER`

---

## Dependency Injection

Memory backends are injected into domain contexts at initialization:

```c
snd_ldr_pe_ctx_t ctx = {0};
ctx.mem_api = &snd_mem_sys;  // or &snd_mem_nt, &snd_mem_win
```

The loader engine calls `ctx.mem_api->alloc`, `ctx.mem_api->protect`, and `ctx.mem_api->free` throughout the reflective loading pipeline. See [Dependency Injection](../../../architecture/dependency_injection.md) for the full pattern.

# Memory: API Reference

This page documents the memory capabilities exported by the framework.

---

## Memory API Instances (`sindri/primitives/memory.h`)

Two pre-built instances of the `snd_memory_api_t` interface are exported globally. These pointers are typically assigned to `ctx.mem_api` during loader or injector initialization.

| Symbol | Backend Paradigm |
|---|---|
| `snd_mem_win` | Standard Win32 API (`VirtualAlloc`, `VirtualProtect`) |
| `snd_mem_native` | Direct Syscalls (`NtAllocateVirtualMemory`, `NtProtectVirtualMemory`) |

---

## The Interface (`sindri/primitives/os_api.h`)

### `snd_memory_api_t`

The function pointer table defining the memory contract. Operators can implement their own instances of this structure to route memory operations through custom mechanisms (e.g., hypervisors, vulnerable drivers).

| Field | Signature | Description |
|---|---|---|
| `alloc` | `snd_status_t (*)(PVOID* base, SIZE_T size, ULONG alloc_type, ULONG protect)` | Allocates virtual memory |
| `free` | `snd_status_t (*)(PVOID base, SIZE_T size, ULONG free_type)` | Frees virtual memory |
| `protect` | `snd_status_t (*)(PVOID base, SIZE_T size, ULONG new_protect, ULONG* old_protect)` | Modifies page protections |

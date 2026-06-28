# Common: API Reference

Public API under `include/sindri/common/`. Include `sindri/common.h` for the full surface.

Implementation sources: `src/common/` (`buffer.c`, `hash.c`, `status.c`, `disk.c`, `debug.c`, `crt_manifest.c`).

---

## Macros (`sindri/common/macros.h`)

| Macro | Description |
|---|---|
| `SND_BEGIN_EXTERN_C` / `SND_END_EXTERN_C` | C++ `extern "C"` linkage wrappers |
| `SND_FORCE_INLINE` | `static __forceinline` (MSVC) or `always_inline` (GCC/Clang) |

---

## Memory (`sindri/common/memory.h`)

Header-only CRT replacements and bounds helpers.

### `SND_PTR_ADD(base, offset)`

Adds a byte offset to a base pointer (casts through `unsigned char *`).

### `snd_memory_bounds_check`

```c
SND_FORCE_INLINE int snd_memory_bounds_check(size_t total_size, size_t offset, size_t size);
```

Returns `1` if `[offset, offset+size)` fits within `[0, total_size)`.

### `snd_memory_ptr_bounds_check`

```c
SND_FORCE_INLINE int snd_memory_ptr_bounds_check(
    const void *base, size_t total_size, const void *ptr, size_t size);
```

Validates that `[ptr, ptr+size)` lies within `[base, base+total_size)`. Returns `0` if `base` or `ptr` is NULL, or if `ptr < base`.

### `snd_memzero`

Zeroes `size` bytes at `dest` using a `volatile` loop (prevents compiler elision). Replaces `memset`.

### `snd_memcpy`

Copies `count` bytes from `src` to `dest`. Replaces `memcpy`.

---

## String (`sindri/common/string.h`)

Header-only bounded string operations.

### ASCII

| Function | Replaces | Notes |
|---|---|---|
| `snd_strnlen(str, max_len)` | `strnlen` | |
| `snd_strncpy(dest, dest_size, src, max_src_len)` | `strncpy` | Always null-terminates |
| `snd_strncat(dest, dest_size, src, max_src_len)` | `strncat` | Bounded append |
| `snd_strnchr(str, c, max_len)` | `strchr` | Bounded search |
| `snd_strncmp(s1, s2, max_len)` | `strncmp` | Returns `<0 / 0 / >0` |

### Wide (UTF-16)

| Function | Replaces | Notes |
|---|---|---|
| `snd_wcsnlen(str, max_len)` | `wcsnlen` | |
| `snd_wcsnicmp(s1, s2, n)` | `wcsnicmp` | Case-insensitive |
| `snd_wcsncpy(dest, dest_size, src, max_src_len)` | `wcsncpy` | |
| `snd_wcsncat(dest, dest_size, src, max_src_len)` | `wcsncat` | |
| `snd_ascii_to_wide(dest, dest_size, src, max_src_len)` | `mbstowcs` | Latin ASCII → wide |

Used by PEB walking, export forwarders, and `LdrLoadDll` path construction.

---

## Buffer (`sindri/common/buffer.h`)

### `snd_buffer_t`

```c
typedef struct snd_buffer_s {
    void       *data;
    size_t      size;
    snd_free_cb free_routine;
} snd_buffer_t;
```

### `snd_free_cb`

```c
typedef void (*snd_free_cb)(snd_buffer_t *buf);
```

### `snd_buffer_init`

Initializes tracking metadata without allocating memory.

```c
void snd_buffer_init(snd_buffer_t *buf, void *data, size_t size, snd_free_cb free_routine);
```

### `snd_buffer_free`

Invokes `free_routine` if set, then zeroes the struct.

```c
void snd_buffer_free(snd_buffer_t *buf);
```

**Source:** `src/common/buffer.c`

### `snd_buffer_bounds_check`

```c
SND_FORCE_INLINE int snd_buffer_bounds_check(const snd_buffer_t *buf, size_t offset, size_t size);
```

Returns `0` if `buf` is NULL, `buf->data` is NULL, or `buf->size` is zero.

### Pre-built free routines

| Function | Backend |
|---|---|
| `snd_buffer_free_heap` | `HeapFree(GetProcessHeap(), …)` |
| `snd_buffer_free_virtual` | `VirtualFree(…, MEM_RELEASE)` |
| `snd_buffer_free_mapped` | `UnmapViewOfFile` |

---

## Hashing (`sindri/common/hash.h`)

Runtime hashing uses the algorithm selected at CMake configure time.

### `snd_hash`

```c
uint32_t snd_hash(const char *str);
```

Case-sensitive. Export names from the EAT.

### `snd_hash_lower`

```c
uint32_t snd_hash_lower(const char *str);
```

Lowercases ASCII before hashing. Import DLL names.

### `snd_hash_wide_lower`

```c
uint32_t snd_hash_wide_lower(const wchar_t *str);
```

Lowercases wide string before hashing. PEB module names.

**Source:** `src/common/hash.c`

---

## Status (`sindri/common/status.h`)

### `snd_status_t`

| Field | Debug (`SND_DEBUG=1`) | Release |
|---|---|---|
| `code` | `snd_status_code_t` | same |
| `os_error` | `int` | same |
| `file` | source file | *(omitted)* |
| `line` | line number | *(omitted)* |
| `context` | 128-byte formatted string | *(omitted)* |

### Convenience macros

| Macro | Description |
|---|---|
| `SND_SUCCEEDED(x)` | `(x.code == SND_SUCCESS)` |
| `SND_FAILED(x)` | `(x.code != SND_SUCCESS)` |
| `SND_OK` | Success status |
| `SND_ERR(code)` | Framework error |
| `SND_ERR_CTX(code, fmt, …)` | Error with formatted context (debug only) |
| `SND_ERR_W32(code)` | Error + `GetLastError()` |
| `SND_ERR_W32_CTX(code, fmt, …)` | W32 + context |
| `SND_ERR_NT(code, nt_status)` | Error + NTSTATUS |
| `SND_ERR_NT_CTX(code, nt_status, fmt, …)` | NT + context |

### `SND_MAX_CTX_LEN`

**Value:** `128` — max formatted context string length.

### Status code ranges (selected)

| Range | Domain |
|---|---|
| `0x100` | Command-line errors |
| `0x200` | File / I/O |
| `0x300` | PE parsing |
| `0x400` | Reflective loading |
| `0x500` | Syscall resolution |
| `0x600` | PEB |
| `0x800` | OS / mapping |
| `0x900` | Access denied |

Full enum: `include/sindri/common/status.h`.

### `snd_status_to_string`

```c
const char *snd_status_to_string(snd_status_t status);
```

Human-readable description. Returns empty string when `SND_DEBUG=0`.

### `snd_status_print`

```c
void snd_status_print(snd_status_t status);
```

Prints code, context, OS error, and file/line via `SND_DEBUG_PRINT`. No-op body when `SND_DEBUG=0`.

**Source:** `src/common/status.c`

---

## Debug (`sindri/common/debug.h`)

Controlled by CMake `SND_ENABLE_DEBUG` → `SND_DEBUG`.

### `SND_DEBUG_PRINT(fmt, …)`

Formatted debug output. No-op when `SND_DEBUG=0`.

### `SND_FDEBUG_PRINT(stream, fmt, …)`

Stream-targeted output when `SND_USE_PRINTF=1`; otherwise same as `SND_DEBUG_PRINT`.

### `SND_FALLBACK_STR(debug_str)`

Returns `debug_str` when `SND_DEBUG=1`, `""` when `SND_DEBUG=0`. Used for stage name strings in state machines.

### `snd_dump_hex`

```c
void snd_dump_hex(const void *dat, size_t len_dat, uintptr_t base_off);
```

Hex + ASCII dump. Only emits when `SND_DEBUG=1`.

**Source:** `src/common/debug.c`

### `SND_DEBUG_MAX_LEN`

**Value:** `1024` — internal format buffer when using `OutputDebugStringA` path.

---

## Disk I/O (`sindri/common/disk.h`)

### `snd_disk_buffer_load`

```c
snd_status_t snd_disk_buffer_load(const char *path, snd_buffer_t *out_buf);
```

Reads entire file into heap memory. Sets `free_routine = snd_buffer_free_heap`.

**Returns:** `SND_OK`, `SND_STATUS_INVALID_PATH`, `SND_STATUS_FILE_CREATE_FAILED`, `SND_STATUS_FILE_READ_FAILED`, etc.

**Source:** `src/common/disk.c`

---

## CRT manifest (`src/common/crt_manifest.c`)

Global linker symbols (not for direct use):

```c
void *memcpy(void *dest, const void *src, size_t count);
void *memset(void *dest, int c, size_t count);
```

Compiled when building CRT-less targets. Satisfies MSVC implicit codegen under `/NODEFAULTLIB`.

---

## Internal NT layouts (`include/sindri/internal/nt/`)

Not part of `sindri/common.h`, but replaces the former `internal/nt_defs.h` monolith.

### `types.h`

| Symbol | Description |
|---|---|
| `SND_NT_SUCCESS(s)` | `((NTSTATUS)(s)) >= 0` |
| `SND_PAGE_SIZE` | `0x1000` |
| `SND_OBJ_CASE_INSENSITIVE` | `0x00000040L` |
| `SND_InitializeObjectAttributes(p, n, a, r, s)` | Init object attributes macro |
| `SND_UNICODE_STRING` | NT counted wide string |
| `SND_OBJECT_ATTRIBUTES` | Object Manager attributes |
| `SND_CLIENT_ID` | Process/thread identifier |
| `snd_init_unicode_string(us, buf, char_count)` | Inline unicode string init |

### `api.h`

Function pointer typedefs: `SND_LdrLoadDll_t`, `SND_NtOpenSection_t`, `SND_NtMapViewOfSection_t`, `SND_NtClose_t`, `SND_NtAllocateVirtualMemory_t`, `SND_NtProtectVirtualMemory_t`, `SND_NtFreeVirtualMemory_t`, `SND_NtOpenProcess_t`, `SND_NtWriteVirtualMemory_t`, `SND_NtCreateThreadEx_t`.

### `peb.h`

Layouts: `SND_PEB`, `SND_PEB_LDR_DATA`, `SND_LDR_DATA_TABLE_ENTRY`, `SND_RTL_USER_PROCESS_PARAMETERS`, `SND_CURDIR`.

Include via `sindri/internal/nt.h`.

---

## Related documentation

- [Infrastructure concepts](infrastructure.md)
- [Status system architecture](../architecture/status_system.md)
- [Hash manifest](../config/hashes_manifest.md)

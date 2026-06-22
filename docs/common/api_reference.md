# Common: API Reference

This page documents the public API surface of the shared infrastructure headers under `include/sindri/common/`.

---

## Buffer Management (`sindri/common/buffer.h`)

### `snd_buffer_t`

Tracked memory buffer structure. Pairs a data pointer with its size and an optional deallocation callback.

| Field | Type | Description |
|---|---|---|
| `data` | `LPVOID` | Pointer to the memory block |
| `size` | `SIZE_T` | Size of the memory block in bytes |
| `free_routine` | `snd_free_cb` | Optional callback invoked by `snd_buffer_free` to release the memory |

---

### `snd_buffer_init`

Initializes a buffer structure with tracking metadata.

| Parameter | Type | Description |
|---|---|---|
| `buf` | `snd_buffer_t*` | Buffer structure to initialize |
| `data` | `LPVOID` | Pointer to the memory block |
| `size` | `SIZE_T` | Size of the memory block |
| `free_routine` | `snd_free_cb` | Optional cleanup callback; may be NULL |

**Returns:** `void`

---

### `snd_buffer_free`

Invokes the buffer's assigned `free_routine` (if non-NULL) and zeroes the structure fields.

| Parameter | Type | Description |
|---|---|---|
| `buf` | `snd_buffer_t*` | Buffer to free and zero |

**Returns:** `void`

---

### Pre-built Free Routines

| Function | Backend | Use Case |
|---|---|---|
| `snd_buffer_free_heap` | `HeapFree(GetProcessHeap(), ...)` | Buffers allocated via `HeapAlloc` |
| `snd_buffer_free_virtual` | `VirtualFree(..., MEM_RELEASE)` | Buffers allocated via `VirtualAlloc` |
| `snd_buffer_free_mapped` | `UnmapViewOfFile(...)` | Buffers created via `MapViewOfFile` |

---

## Bounds Checking (`sindri/common/helpers.h`)

All bounds-checking functions are force-inlined to eliminate call overhead.

### `snd_bounds_check`

Pure arithmetic bounds validation. Returns `TRUE` if `offset + size` fits within `total_size`.

| Parameter | Type | Description |
|---|---|---|
| `total_size` | `SIZE_T` | Total size of the region |
| `offset` | `SIZE_T` | Offset from the start |
| `size` | `SIZE_T` | Size of the sub-region to validate |

**Returns:** `BOOL`

---

### `snd_buffer_bounds_check`

Validates an `(offset, size)` pair against a tracked `snd_buffer_t`. Returns `FALSE` if the buffer is NULL, empty, or the region exceeds bounds.

| Parameter | Type | Description |
|---|---|---|
| `buf` | `const snd_buffer_t*` | Tracked buffer to validate against |
| `offset` | `SIZE_T` | Offset from `buf->data` |
| `size` | `SIZE_T` | Size of the sub-region |

**Returns:** `BOOL`

---

### `snd_ptr_bounds_check`

Validates an arbitrary pointer against a known base region. Computes the offset from `base` to `ptr` and delegates to `snd_bounds_check`.

| Parameter | Type | Description |
|---|---|---|
| `base` | `const void*` | Base pointer of the known region |
| `total_size` | `SIZE_T` | Total size of the region |
| `ptr` | `const void*` | Target pointer to validate |
| `size` | `SIZE_T` | Size of the access at the target pointer |

**Returns:** `BOOL`

---

## CRT Replacement Primitives (`sindri/common/helpers.h`)

### `snd_memzero`

Zeroes `size` bytes at `dest` using a `volatile BYTE*` loop to prevent compiler elision.

| Parameter | Type | Description |
|---|---|---|
| `dest` | `void*` | Destination pointer |
| `size` | `size_t` | Number of bytes to zero |

---

### `snd_memcpy`

Copies `count` bytes from `src` to `dest`. No alignment assumptions.

| Parameter | Type | Description |
|---|---|---|
| `dest` | `void*` | Destination pointer |
| `src` | `const void*` | Source pointer |
| `count` | `size_t` | Number of bytes to copy |

---

### `snd_strnlen`

Returns the length of `str`, up to a maximum of `max_len`.

| Parameter | Type | Description |
|---|---|---|
| `str` | `const char*` | Input string |
| `max_len` | `size_t` | Maximum characters to scan |

**Returns:** `size_t`

---

### `snd_strncpy`

Copies up to `max_src_len` characters from `src` into `dest`, always null-terminating. Never writes more than `dest_size` bytes.

| Parameter | Type | Description |
|---|---|---|
| `dest` | `char*` | Destination buffer |
| `dest_size` | `size_t` | Total size of the destination buffer |
| `src` | `const char*` | Source string |
| `max_src_len` | `size_t` | Maximum characters to copy from source |

---

### `snd_strncat`

Appends up to `max_src_len` characters from `src` to `dest`, respecting `dest_size` and always null-terminating.

| Parameter | Type | Description |
|---|---|---|
| `dest` | `char*` | Destination buffer (must already be null-terminated) |
| `dest_size` | `SIZE_T` | Total size of the destination buffer |
| `src` | `const char*` | Source string to append |
| `max_src_len` | `SIZE_T` | Maximum characters to append |

---

### `snd_strnchr`

Searches for character `c` within the first `max_len` characters of `str`.

| Parameter | Type | Description |
|---|---|---|
| `str` | `const char*` | String to search |
| `c` | `char` | Character to find |
| `max_len` | `SIZE_T` | Maximum characters to scan |

**Returns:** `const char*` — pointer to the first occurrence, or `NULL`.

---

### `snd_strncmp`

Compares up to `max_len` characters of `s1` and `s2`.

| Parameter | Type | Description |
|---|---|---|
| `s1` | `const char*` | First string |
| `s2` | `const char*` | Second string |
| `max_len` | `SIZE_T` | Maximum characters to compare |

**Returns:** `int` — `<0` if `s1 < s2`, `0` if equal, `>0` if `s1 > s2`.

---

## Hashing (`sindri/common/hash.h`)

### `snd_hash`

Computes the configured hash of an ASCII string. Case-sensitive.

| Parameter | Type | Description |
|---|---|---|
| `str` | `const char*` | Null-terminated ASCII string |

**Returns:** `DWORD` — computed hash value.

**Primary use:** Hashing export names from the PE Export Address Table (e.g., `NtAllocateVirtualMemory`).

---

### `snd_hash_lower`

Computes the configured hash of an ASCII string after lowering each character.

| Parameter | Type | Description |
|---|---|---|
| `str` | `const char*` | Null-terminated ASCII string |

**Returns:** `DWORD` — computed hash value.

**Primary use:** Hashing DLL names from the PE Import Directory (e.g., `KERNEL32.dll`).

---

### `snd_hash_wide_lower`

Computes the configured hash of a wide (UTF-16) string after lowering each character.

| Parameter | Type | Description |
|---|---|---|
| `str` | `const wchar_t*` | Null-terminated wide string |

**Returns:** `DWORD` — computed hash value.

**Primary use:** Hashing module names from the PEB's `InMemoryOrderModuleList` (e.g., `L"ntdll.dll"`).

---

## Disk I/O (`sindri/common/disk.h`)

### `snd_buffer_load_from_disk`

Reads an entire file into a heap-allocated `snd_buffer_t`. The buffer's `free_routine` is set to `snd_buffer_free_heap`.

| Parameter | Type | Description |
|---|---|---|
| `path` | `const char*` | Null-terminated file path |
| `out_buf` | `snd_buffer_t*` | Output buffer; must be zeroed before the call |

**Returns:** `snd_status_t` — `SND_OK` on success, or an I/O error (`SND_STATUS_INVALID_PATH`, `SND_STATUS_FILE_CREATE_FAILED`, `SND_STATUS_FILE_READ_FAILED`, etc.).

> [!NOTE]
> The caller is responsible for releasing the buffer via `snd_buffer_free(out_buf)`.

---

## Debug Output (`sindri/common/helpers.h`)

### `SND_DEBUG_PRINT(fmt, ...)`

Macro that emits a formatted debug string. Compiles to a no-op when `SND_DEBUG=0`.

### `SND_FDEBUG_PRINT(stream, fmt, ...)`

Macro that emits a formatted debug string to a specific file stream. Only meaningful when `SND_USE_PRINTF=1`; otherwise routes to `OutputDebugStringA`.

### `snd_dump_hex`

Prints a combined hexadecimal and ASCII view of a byte buffer. Output is gated by `SND_DEBUG`.

| Parameter | Type | Description |
|---|---|---|
| `dat` | `const void*` | Pointer to the data to dump |
| `len_dat` | `size_t` | Number of bytes to dump |
| `base_off` | `ULONG_PTR` | Base address for computing printed offsets |

### `SND_DEBUG_MAX_LEN`

**Value:** `1024`

Maximum length of a formatted debug string buffer used internally by `snd_debug_print_internal`.

---

### `snd_debug_print_internal`

Internal function invoked by `SND_DEBUG_PRINT` and `SND_FDEBUG_PRINT` to safely format and print debug strings up to `SND_DEBUG_MAX_LEN`. Stripped when `SND_DEBUG=0`.

---

## Status System (`sindri/common/status.h`)

### `SND_MAX_CTX_LEN`

**Value:** `128`

Maximum length of the formatted error context string embedded within the `snd_status_t` structure.

---

### `snd_status_print`

Translates a `snd_status_t` code into a human-readable string using the framework's global status translation table. Also prints the context buffer if one is present.

| Parameter | Type | Description |
|---|---|---|
| `status` | `snd_status_t` | The status code/struct to translate and print |

**Returns:** `void`

---

## Internal NT Definitions (`sindri/internal/nt_defs.h`)

While primarily internal, the following constants and macros are explicitly defined to avoid dependency on the massive `<winternl.h>` header.

### `SND_PAGE_SIZE`

**Value:** `0x1000` (4096 bytes)

Standard x86/x64 memory page size.

### `SND_OBJ_CASE_INSENSITIVE`

**Value:** `0x00000040L`

Object Manager flag indicating case-insensitive string evaluation.

### `SND_InitializeObjectAttributes`

Macro that zeroes and initializes an `OBJECT_ATTRIBUTES` structure with the provided Root Directory, Object Name, and Attributes. Mirrors the standard `InitializeObjectAttributes` macro.

---

## Utility Macros (`sindri/common/helpers.h`)

### `SND_PTR_ADD(base, offset)`

Safely adds a byte offset to a base pointer by casting through `BYTE*`.

```c
PVOID target = SND_PTR_ADD(image_base, rva);
```

### `SND_FORCE_INLINE`

Compiler-agnostic macro for forcing function inlining. Expands to `static __forceinline` on MSVC and `static inline __attribute__((always_inline))` on GCC/Clang.

### `SND_BEGIN_EXTERN_C` / `SND_END_EXTERN_C`

C++ linkage compatibility wrappers. Expand to `extern "C" { ... }` when compiled as C++, empty otherwise.

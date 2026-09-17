# Files Primitive

The file primitives domain abstracts local file reading operations. These APIs provide simple loading of file contents into memory buffers (`snd_buffer_t`), which is primarily used by loaders (PE/COFF loaders) and the `unified` command line tools.

## Backends

| Backend | Implementation | Description | Path Requirement |
|---|---|---|---|
| `snd_file_win` | `src/primitives/files/win.c` | Uses Win32 `CreateFileA` and `ReadFile`. | Supports relative and absolute paths. |
| `snd_file_nt` | `src/primitives/files/nt.c` | Uses Native API `NtCreateFile` and `NtReadFile` via ntdll exports. | **Absolute paths only**. |
| `snd_file_sys` | `src/primitives/files/sys.c` | Uses Direct Syscalls for `NtCreateFile` and `NtReadFile`. | **Absolute paths only**. |

> [!WARNING]
> **Path Resolution:** When using the Native (`snd_file_nt`) or Syscall (`snd_file_sys`) backends, the `path` argument MUST be a fully qualified absolute path (e.g. `C:\path\to\file.ext`). Relative paths are not supported by the NT file APIs without providing a root directory handle, and will fail with `0xC000003A` (`STATUS_OBJECT_PATH_NOT_FOUND`). The Win32 backend (`snd_file_win`) fully supports relative paths by resolving them against the current working directory.

## Usage

Example of using the file primitive:

```c
#include <sindri/primitives/files.h>

snd_buffer_t file_buf = {0};

// Use the appropriate backend:
// snd_status_t status = snd_file_win.load("payload.dll", &file_buf);
snd_status_t status = snd_file_sys.load("C:\\Windows\\System32\\notepad.exe", &file_buf);

if (SND_SUCCEEDED(status)) {
    // file_buf.data contains the file contents
    // file_buf.size contains the file size
    
    // ... use the buffer ...
    
    snd_buffer_free(&file_buf);
}
```

## Table of Contents

- [internals.md](internals.md) — per-backend mechanics, buffer ownership, path prefixing
- [API reference](../../api_reference.md#files) — `snd_file_api_t`, `snd_file_load_cb`, and backend declarations

## Related documentation

- [Common API reference](../../api_reference.md) — `snd_buffer_t` lifecycle
- [Getting started: basic usage](../../getting_started/basic_usage.md) — file loading in bootstrapping examples

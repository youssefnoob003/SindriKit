# Injection: API Reference

Public injection API under `include/sindri/injection/`. Include `sindri/injection.h` or `sindri.h` for the full surface.

---

## Shared Context (`sindri/injection/context.h`)

All injection techniques operate on `snd_inj_ctx_t`. Defined in `context.h`, not in technique-specific headers.

### `snd_inj_ctx_t`

| Field | Type | Description |
|---|---|---|
| `target_pid` | `DWORD` | PID of the target process |
| `target_process` | `HANDLE` | Handle opened via `proc_api->open_process` |
| `remote_base` | `PVOID` | Remote allocation base |
| `remote_entry_point` | `PVOID` | Optional. If set, thread starts here instead of `remote_base` (PE path) |
| `remote_size` | `SIZE_T` | Size of the remote allocation |
| `remote_thread` | `HANDLE` | Handle from `proc_api->create_remote_thread` |
| `stage` | `snd_inj_stage_t` | Current pipeline stage |
| `payload` | `const snd_buffer_t *` | Source buffer (shellcode or locally baked PE image) |
| `proc_api` | `const snd_process_api_t *` | Injected remote process API |

**Required before any chain:** `target_pid`, `proc_api`, and (for alloc/write) a valid `payload`.

---

### `snd_inj_stage_t`

| Constant | Value | Description |
|---|---|---|
| `SND_INJ_STAGE_UNINITIALIZED` | 0 | Not started |
| `SND_INJ_STAGE_TARGET_ACQUIRED` | 1 | Process handle obtained |
| `SND_INJ_STAGE_MEMORY_ALLOCATED` | 2 | Remote memory allocated (RW) |
| `SND_INJ_STAGE_PAYLOAD_WRITTEN` | 3 | Payload written remotely |
| `SND_INJ_STAGE_PROTECTIONS_SET` | 4 | Remote memory set to RX |
| `SND_INJ_STAGE_EXECUTED` | 5 | Remote thread created |

---

### `snd_inj_stage_to_string`

Returns a human-readable stage name for logging and error context.

```c
const char *snd_inj_stage_to_string(snd_inj_stage_t stage);
```

**Source:** `src/injection/context.c`

---

### `snd_inj_cleanup`

Closes `remote_thread` and `target_process` through `proc_api->close_handle`, clears remote fields, resets stage to `UNINITIALIZED`.

```c
void snd_inj_cleanup(snd_inj_ctx_t *ctx);
```

Does not free loader-local mappings. No-op if `ctx` or `proc_api` is NULL.

---

## Classic Engine (`sindri/injection/classic/engine.h`)

Per-stage functions with strict stage validation. Each advances `ctx->stage` on success.

| Function | Required stage | `proc_api` callback | Advances to |
|---|---|---|---|
| `snd_inj_classic_open_target` | `UNINITIALIZED` | `open_process` | `TARGET_ACQUIRED` |
| `snd_inj_classic_alloc_remote` | `TARGET_ACQUIRED` | `alloc_remote` | `MEMORY_ALLOCATED` |
| `snd_inj_classic_write_payload` | `MEMORY_ALLOCATED` | `write_remote` | `PAYLOAD_WRITTEN` |
| `snd_inj_classic_set_protections` | `PAYLOAD_WRITTEN` | `protect_remote` | `PROTECTIONS_SET` |
| `snd_inj_classic_execute` | `PROTECTIONS_SET` | `create_remote_thread` | `EXECUTED` |

### `snd_inj_classic_open_target`

Opens the target with `PROCESS_ALL_ACCESS` (`0x001FFFFF`). Requires non-zero `target_pid`.

**Returns:** `SND_OK`, `SND_STATUS_PROCESS_OPEN_FAILED`, `SND_STATUS_INVALID_STAGE_SEQUENCE`, `SND_STATUS_INVALID_PARAMETER`

---

### `snd_inj_classic_alloc_remote`

Allocates `payload->size` bytes remotely with `MEM_COMMIT | MEM_RESERVE` and `PAGE_READWRITE`. Sets `remote_size = payload->size`.

**Returns:** `SND_OK`, allocation errors from `proc_api`, `SND_STATUS_INVALID_STAGE_SEQUENCE`

---

### `snd_inj_classic_write_payload`

Writes `payload->data` (`remote_size` bytes) to `remote_base`.

**Returns:** `SND_OK`, `SND_STATUS_VIRTUAL_WRITE_FAILED`, `SND_STATUS_INVALID_STAGE_SEQUENCE`

---

### `snd_inj_classic_set_protections`

Transitions the full remote region to `PAGE_EXECUTE_READ`.

**Returns:** `SND_OK`, `SND_STATUS_VIRTUAL_PROTECT_FAILED`, `SND_STATUS_INVALID_STAGE_SEQUENCE`

---

### `snd_inj_classic_execute`

Creates a remote thread at `remote_base` with parameter `NULL`. Stores handle in `remote_thread`.

**Returns:** `SND_OK`, `SND_STATUS_THREAD_CREATE_FAILED`, `SND_STATUS_INVALID_STAGE_SEQUENCE`

**Source:** `src/injection/classic/engine.c`

---

## Classic Chain (`sindri/injection/classic/chain.h`)

### `snd_inj_classic_shell`

Runs the full shellcode pipeline: open → alloc → write → protect → execute.

```c
snd_status_t snd_inj_classic_shell(snd_inj_ctx_t *ctx);
```

| Parameter | Description |
|---|---|
| `ctx` | Context with `target_pid`, `payload`, and `proc_api` set |

**Returns:** `SND_OK` or the failing stage's status

**Source:** `src/injection/classic/chain.c`

---

### `snd_inj_classic_pe`

Orchestrates local PE preparation via `snd_ldr_pe_ctx_t` and remote injection via `snd_inj_ctx_t`. See [techniques.md](techniques.md) for the interleaved step order.

```c
snd_status_t snd_inj_classic_pe(snd_ldr_pe_ctx_t *ldr_ctx, snd_inj_ctx_t *inj_ctx);
```

| Parameter | Description |
|---|---|
| `ldr_ctx` | Loader context with `raw_source`, `mem_api`, and `mod_api` set |
| `inj_ctx` | Injection context with `target_pid` and `proc_api` set |

On success, `inj_ctx` reaches `SND_INJ_STAGE_EXECUTED` with `remote_entry_point` pointing at the remote entry point and `remote_base` pointing at the allocation base.

**Returns:** `SND_OK` or the failing loader/injection stage status

**Source:** `src/injection/classic/chain.c`

---

## Related Documentation

- [Loader API](../loaders/api_reference.md) — `snd_ldr_pe_ctx_t` used by `snd_inj_classic_pe`
- [Process primitives](../primitives/process/api_reference.md) — `snd_process_api_t` backends
- [State machines](../../architecture/state_machines.md) — global stage pattern

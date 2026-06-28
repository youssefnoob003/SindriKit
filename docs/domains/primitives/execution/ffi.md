# Dynamic Invocation FFI

The Foreign Function Interface (FFI) layer invokes dynamically resolved function pointers at runtime using a generic argument array. PoCs use it for **named exports** (`-e`/`-a`) whose signatures are unknown at compile time. DllMain and EXE entry use typed calls inside the loader engine — see [Loader integration](#loader-integration).

**Public API:** `snd_ffi_execute` in `include/sindri/primitives/ffi.h`  
**Dispatch:** `src/primitives/execution/ffi/ffi_invoke.c`  
**Bridges:** `src/primitives/execution/ffi/asm/`

---

## Why not cast in C?

Calling an arbitrary pointer in C requires casting to a typed function signature. MSVC emits **C4152** (non-standard function pointer conversion) when casting between incompatible function pointer types.

SindriKit avoids this at the call site:

1. Resolve the export to `FARPROC` / `UINT_PTR`.
2. Cast through `(PVOID)(UINT_PTR)proc` — an integer conversion, not a function-pointer conversion.
3. Pass to `snd_ffi_execute`, which type-puns inside the MASM stub where the CPU simply `CALL`s the raw address.

PoC pattern (from `pocs/loader_winapi/main.c`):

```c
FARPROC dynamic_proc = NULL;
status = snd_ldr_pe_get_proc_address(&ctx, export_name, &dynamic_proc);

PVOID    fn_ptr = (PVOID)(UINT_PTR)dynamic_proc;
UINT_PTR retval = snd_ffi_execute(fn_ptr, call_argc, call_argc > 0 ? call_args : NULL);
```

---

## Validation (`ffi_invoke.c`)

Before entering assembly:

| Check | Action |
|---|---|
| `pFunctionAddress == NULL` | Return `0` |
| `dwArgCount > 0 && pArgs == NULL` | Return `0` |
| Otherwise | Route to arch-specific bridge |

There is no arity or type validation — mismatched arguments corrupt the stack or registers.

---

## x64 bridge (`snd_ffi_bridge_x64`)

File: `ffi/asm/ffi_x64.asm`

Follows the Microsoft x64 calling convention:

1. **Registers:** Arguments 1–4 load into `RCX`, `RDX`, `R8`, `R9` from `pArgs[0..3]`.
2. **Stack:** Arguments 5+ spill above the mandatory **32-byte shadow space**.
3. **Alignment:** Dynamic frame allocation keeps `RSP` 16-byte aligned before `CALL`.
4. **Preservation:** Non-volatile registers saved in the prologue and restored on return.

The bridge computes frame size as `ALIGN_UP(32 + max(0, count-4)*8, 16)`.

---

## x86 bridge (`snd_ffi_bridge_x86`)

File: `ffi/asm/ffi_x86.asm`

1. Push arguments **right-to-left** (reverse iteration over `pArgs`).
2. `CALL` the target.
3. Restore `ESP` to its pre-push value (`lea esp, [ebp-12]`).

This supports both **`__cdecl`** (caller cleans) and **`__stdcall`** (callee cleans) targets without knowing the convention in advance — the bridge always resets the stack pointer after return.

---

## Loader integration

| Stage | Mechanism |
|---|---|
| EXE entry | Typed function pointer in `snd_ldr_pe_execute_image` (`chain.c`) |
| DLL `DllMain` | Typed `snd_dll_entry_proc_t` call in `snd_ldr_pe_execute_image` |
| TLS callbacks | Direct `PIMAGE_TLS_CALLBACK` invocations in `snd_ldr_pe_execute_tls_callbacks` |
| Named export (`-e`) | PoC resolves with `snd_ldr_pe_get_proc_address`, then `snd_ffi_execute` |

See [Loaders techniques](../../loaders/techniques.md) for the full reflective pipeline.

---

## Limitations

- **No floating-point or struct returns** beyond what fits in `UINT_PTR` / integer registers.
- **No varargs** — pass a fixed count matching the target.
- **x86 vs x64** — build architecture selects the bridge; there is no cross-bitness FFI (use Heaven's Gate for WoW64 → x64).

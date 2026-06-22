# The Status System

SindriKit utilizes a universal status reporting mechanism built around the `snd_status_t` structure. Traditional C error handling (returning raw integers or `NULL` pointers) is often insufficient for complex offensive tooling, as it fails to capture critical contextual information (such as the underlying OS error code) when a failure occurs deep within an operation. 

The SindriKit status system is designed to provide maximum verbosity during development while guaranteeing absolute zero-footprint string literal emissions during production deployments.

## The `snd_status_t` Structure

Every function in the framework that can fail returns a `snd_status_t` object (defined in `include/sindri/common/status.h`).

The structure tracks the following:
1. `code`: A unified `snd_status_code_t` enumeration detailing the exact framework-level error (e.g., `SND_STATUS_NT_HEADERS_TRUNCATED`, `SND_STATUS_SSN_NOT_FOUND`).
2. `os_error`: The native OS error code captured at the moment of failure (via `GetLastError()` or an NTSTATUS value).

## Development vs. Production Tiers (OpSec)

The true power of the status system lies in how it interacts with the `SND_DEBUG` preprocessor macro (controlled via the `SND_ENABLE_DEBUG` CMake variable).

### Debug Tier (`SND_DEBUG=1`)
During development, the `snd_status_t` structure expands to include heavily detailed telemetry:
- `file`: The C source file where the error occurred (`__FILE__`).
- `line`: The exact line number (`__LINE__`).
- `context`: A dynamically formatted string buffer (up to 128 chars) providing hyper-specific details (e.g., "Failed to resolve API hash 0xDEADBEEF").

This allows operators to pinpoint exactly why an evasion primitive or loader sequence failed without relying on external debuggers.

### Silent Tier (`SND_DEBUG=0`)
> [!CAUTION]
> When compiling for an operation, emitting plaintext string literals (like file paths or error contexts) into the payload's `.rdata` section creates massive, easily signatureable IOCs.

When `SND_ENABLE_DEBUG` is disabled, the status macros dynamically shrink the `snd_status_t` struct:
- The `file`, `line`, and `context` fields are completely excised from the struct definition.
- The `SND_ERR_CTX` macro ignores all string formatting arguments.
- The compiled output is reduced to pure integer assignments (just `code` and `os_error`).

This achieves a completely silent, stringless release build without forcing the operator to manually strip out error handling code.

## Helper Macros

Developers interact with the status system using a suite of context-aware macros:

- `SND_OK`: Returns a successful status.
- `SND_ERR(code)`: Returns a generic framework error.
- `SND_ERR_W32(code)`: Returns a framework error and automatically captures `GetLastError()`.
- `SND_ERR_CTX(code, fmt, ...)`: Returns an error with a custom `printf`-style formatted string (which is automatically stripped in Silent builds).
- `SND_ERR_W32_CTX(code, fmt, ...)`: Combines OS error capturing with custom string context.

By strictly adhering to this pattern, all toolkit domains inherit both robust diagnostics and guaranteed operational security.

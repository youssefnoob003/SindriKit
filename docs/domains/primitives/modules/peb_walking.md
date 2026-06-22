# PEB Walking

The modules subdomain provides mechanisms for interacting with the loaded DLLs of a process. The primary challenge in offensive tooling is resolving module base addresses and exported functions without relying on highly monitored APIs like `GetModuleHandle` or `GetProcAddress`.

## Traversing the PEB

The Process Environment Block (PEB) is a userland structure containing extensive process information, including the lists of all currently loaded DLLs. It is accessible without invoking any Win32 or NT API calls.

- **x64:** Accessed via the `gs` segment register at offset `0x60`
- **x86:** Accessed via the `fs` segment register at offset `0x30`

### The Loaded Module Lists

Within the PEB, `PEB->Ldr` points to a `PEB_LDR_DATA` structure. This structure contains three doubly-linked lists of `LDR_DATA_TABLE_ENTRY` nodes:
1. `InLoadOrderModuleList`
2. `InMemoryOrderModuleList`
3. `InInitializationOrderModuleList`

SindriKit traverses the `InMemoryOrderModuleList`. By iterating through these nodes, the framework inspects the `BaseDllName` field of each loaded module.

## Resolution Methods

### String Comparison
The framework provides `snd_peb_get_module_base` to perform a case-insensitive, wide-string comparison against the `BaseDllName` (e.g., matching `L"ntdll.dll"`).

### Hash Comparison
For superior OpSec, `snd_peb_get_module_base_by_hash` computes a runtime hash of each module's name and compares it against a compile-time hash. This completely eliminates plaintext module strings (like `ntdll.dll` or `kernel32.dll`) from the implant's `.rdata` section.

**Reference:** [Finding kernel32 base and function addresses in shellcode](https://www.ired.team/offensive-security/defense-evasion/finding-kernel32-base-and-function-addresses-in-shellcode)

# Syscall Gates

The framework provides four highly-researched techniques for extracting System Service Numbers (SSNs) at runtime. These form the building blocks of the syscall resolution pipeline.

## Hell's Gate (`snd_hell_extract_syscall`)

The baseline technique. On an unhooked system, every `Nt*` / `Zw*` stub in `ntdll.dll` follows a predictable prologue pattern. On x64:

```
4C 8B D1        mov r10, rcx
B8 XX XX 00 00  mov eax, <SSN>
0F 05           syscall
C3              ret
```

Hell's Gate locates the target function by hash in `ntdll`'s export table, walks to its stub, and validates the first four bytes against this pattern (`0x4C 0x8B 0xD1 0xB8`). If the pattern matches, bytes 4–5 contain the SSN as a little-endian `WORD` and are extracted directly. On x86 WOW64, the analogous check is a leading `0xB8` (`mov eax, <SSN>`) at offset 0.

**Original research:** [Hell's Gate](https://github.com/am0nsec/HellsGate) by am0nsec & smelly\_\_vx

---

## Halo's Gate (`snd_halo_extract_syscall`)

Hell's Gate fails when the target stub has been patched. A typical userland hook overwrites the first bytes of the stub with a `JMP` (`0xE9`) redirect to the EDR's trampoline. 

When Halo's Gate detects this (`p[0] == 0xE9`), it abandons the hooked stub and searches adjacent stubs in memory. Syscall stubs for neighboring exports are allocated contiguously in the export table and their SSNs are sequential. By walking up and down from the hooked stub until a clean, pattern-matching neighbor is found, the correct SSN is inferred by applying the neighbor's index offset.

**Original research:** [Halo's Gate](https://blog.sektor7.net/#!res/2021/halosgate.md) by Sektor7

---

## Tartarus' Gate (`snd_tartarus_extract_syscall`)

Extends Halo's Gate to cover a second hook placement pattern. Some EDR products patch stubs with a short jump (`0xEB`, a single-byte relative `JMP`) rather than the full 5-byte `0xE9` variant. 

Tartarus' Gate detects both (`p[0] == 0xE9 || p[0] == 0xEB`) before delegating to the same neighbor-search logic, ensuring coverage of EDR hooks that Halo's Gate alone would miss.

**Original research:** [Tartarus' Gate](https://github.com/trickster0/TartarusGate) by trickster0

---

## VelesReek (`snd_veles_extract_syscall`)

An alternative SSN recovery approach that locates the `syscall` instruction (`0x0F 0x05`) itself within the stub rather than relying solely on the function prologue pattern. 

On hooked stubs where the prologue has been overwritten, the `syscall` instruction and its immediate context can still anchor SSN derivation. Like the Gate variants, SindriKit's implementation also covers `0xE9`, `0xEB`, and (on x86) `0xE8` jump prefixes by falling through to the shared neighbor-search routine.

**Original research:** [VelesReek](https://github.com/klezvirus/VelesReek) by klezvirus

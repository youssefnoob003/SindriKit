# Examples & Proof of Concepts

This directory contains standalone execution wrappers built on top of the SindriKit library.

## Table of Contents
- [loader_winapi.md](loader_winapi.md)
  Walkthrough of standard reflective loading using noisy Win32 APIs for pipeline validation.
- [loader_nowinapi.md](loader_nowinapi.md)
  Walkthrough of evasive loading using direct syscalls, Hell's Gate, and PEB walking.
- [loader_noCRT_nowinapi.md](loader_noCRT_nowinapi.md)
  Walkthrough of the ultimate evasive profile: stripping the MSVC C Runtime entirely for minimal footprint.
- [heavens_gate.md](heavens_gate.md)
  Walkthrough of transitioning a 32-bit WoW64 process into native 64-bit execution.

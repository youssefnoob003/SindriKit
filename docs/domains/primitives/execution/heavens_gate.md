# Heaven's Gate

Heaven's Gate is a specialized execution primitive for transitioning from a 32-bit WoW64 process into native 64-bit mode by manipulating the CPU segment selectors.

## The WoW64 Boundary

When a 32-bit application executes on a 64-bit Windows OS, it runs inside the Windows-on-Windows (WoW64) subsystem. The WoW64 layer acts as an emulator, intercepting 32-bit API calls, thunking their arguments into 64-bit formats, and transitioning execution to the 64-bit kernel.

Security products frequently hook the 32-bit `ntdll.dll` residing inside the WoW64 environment. By initiating a Heaven's Gate transition, an implant bypasses this 32-bit userland completely and directly invokes the 64-bit kernel primitives.

## Segment Selector `0x33`

The transition is achieved by executing a far jump or call using the `0x33` segment selector. In 64-bit Windows, `0x33` is the Code Segment (`CS`) value that indicates the CPU should execute the subsequent instructions in 64-bit long mode.

SindriKit implements this via the `snd_hg_execute_64` function, allowing a 32-bit payload to pass a 64-bit function address and a 64-bit argument array across the boundary and retrieve a 64-bit `RAX` return value.

> [!NOTE]
> The Heaven's Gate primitives are only meaningful when compiled into a 32-bit process running under WoW64 on a 64-bit OS. Attempting to invoke them in a native 64-bit process or a pure 32-bit OS will result in failure.

**Original research:** [Heaven's Gate](https://github.com/JustasMasiulis/wow64pp) — see also [Alex Ionescu's WoW64 internals](https://wbenny.github.io/2018/11/04/wow64-internals.html)

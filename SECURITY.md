# Security Policy

SindriKit is a foundational C library designed exclusively for building offensive security capabilities (implants, loaders, evasion tooling). As such, the definition of a "security vulnerability" differs fundamentally from traditional software projects.

Vulnerabilities in SindriKit are evaluated strictly through the lens of **Operational Security (OpSec)** and **Implant Stability**.

## Legal Disclaimer

**SindriKit is built for educational, research, and authorized Red Teaming purposes only.** 
The authors and contributors assume no liability and are not responsible for any misuse, damage, or illegal actions caused by utilizing this toolkit. By using SindriKit, you agree to abide by all applicable local, state, and federal laws.

## Pre-Compiled Binaries

For operational security (OpSec) reasons, **no pre-compiled binaries are provided** in GitHub Releases or anywhere else in this repository. Distributing raw executables facilitates trivial signature generation by automated defensive scanners. All operators must compile the framework and Proof-of-Concept loaders from source.

## Scope of Vulnerabilities

I consider the following to be legitimate security vulnerabilities within the scope of SindriKit:

- **OpSec Regressions (SILENT Tier):** 
   If the framework is compiled in the production `SILENT` tier (`SND_ENABLE_DEBUG=OFF`), and it unintentionally leaks framework strings, debug artifacts, plaintext API names, or internal error contexts into the final compiled binary (`.rdata` or `.data`), this is a critical OpSec vulnerability. Operators rely on the `SILENT` tier to produce zero-footprint payloads.
   
- **Memory Corruption & Stability:**
   Buffer overflows, out-of-bounds reads/writes in the custom PE parser, or unhandled null pointer dereferences that lead to the crashing of an implant or host process. Implants must be rock-solid; arbitrary crashes generate highly visible process crash dumps and defensive telemetry that instantly burn an operation.

## Out of Scope

The following are **NOT** considered vulnerabilities:

- **AV/EDR Detections:** SindriKit provides the architecture and low-level evasion primitives; it does not guarantee evasion out-of-the-box against all modern telemetry. If an AV/EDR vendor authors a behavioral signature against a provided PoC loader, that is an implementation detection, not a framework vulnerability.
- **Issues in the DEBUG Tier:** The `DEBUG` tier (`SND_ENABLE_DEBUG=ON`) is explicitly designed to be verbose, string-heavy, and noisy for local development. OpSec failures and string artifacts in this tier are expected and intentional.
- **Operator Error:** Misconfiguration of the Dependency Injection (DI) layers, providing invalid architecture targets, or failure to properly bootstrap the execution primitives (e.g., forgetting to configure the `ntdll` base before calling resolvers).

## Reporting a Vulnerability

**Please do not report OpSec or stability vulnerabilities via public GitHub issues.**

If you discover a vulnerability that fits the critical criteria above, please report it by:
1. Opening a **GitHub Security Advisory** draft on this repository. You can do this by navigating to the **Security** tab, selecting **Advisories**, and clicking **Report a vulnerability**, or by going directly to [this link](https://github.com/youssefnoob003/SindriKit/security/advisories/new).
2. Including detailed steps to reproduce the issue.
3. Specifying your exact build configuration (e.g., specific CMake variables used, MSVC compiler version, target architecture x86/x64, and whether `SND_CRTLESS` was active).
4. If reporting an OpSec string leak, please attach the relevant reverse engineering artifacts (such as `strings` output, hex dumps, or a Compiler Explorer link proving the leak).

I will review the advisory and work with you to patch the framework securely while minimizing operational exposure.

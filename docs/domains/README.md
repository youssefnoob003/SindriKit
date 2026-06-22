# Operational Domains

This directory categorizes the actionable offensive capabilities of SindriKit into specialized functional areas.

> [!IMPORTANT]
> **Domain Independence**
> Each domain must remain entirely self-contained. A loader cannot directly depend on an evasion module; they interact exclusively through the injected primitive interfaces.

## Table of Contents
- [primitives/](primitives/)
  The foundational extraction, memory, and resolution techniques that all other domains rely on.
- [loaders/](loaders/)
  Mechanisms for bootstrapping and executing code payloads entirely in memory (e.g., Reflective PE Loading).
- [evasion/](evasion/)
  *Placeholder:* Future domain for heuristic evasion, memory scanning bypasses, and sleep obfuscation.
- [injection/](injection/)
  *Placeholder:* Future domain for remote process targeting, thread hijacking, and payload injection.

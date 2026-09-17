# Build Scripts

Pre-build automation that generates compile-time artifacts, plus repository tooling.

## Table of Contents

- [generate_hashes.md](generate_hashes.md) — hash pipeline, randomized seed, manifest integration
- [mutator.md](mutator.md) — `SND_MORPH` mutation engine (C/ASM polymorphism, struct shuffling)

## Tooling

- `scripts/audit_docs.py` — documentation integrity audit run in CI. Verifies relative links, same-file anchors, balanced code fences, and that every `snd_*` identifier named in the docs exists in the code tree. Run from the repo root:

```sh
python scripts/audit_docs.py
```

## Related documentation

- [Config: hashes manifest](../config/hashes_manifest.md)
- [Common: hashing](../common/infrastructure.md)

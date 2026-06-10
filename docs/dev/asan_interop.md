# GOIR and AddressSanitizer (ASan) Interoperability

## Overview

GOIR (Graded Optics IR) and AddressSanitizer (ASan) can be used together to provide multi-layered safety checks. While ASan focuses on redzone-based spatial safety and quarantine-based temporal safety, GOIR provides metadata-backed safety that is preserved through pointer derivations and memory operations.

## Support Status (Diagnostic Tier)

In the `diag` tier, GOIR and ASan are compatible with the following considerations:

1.  **Link Order**: `libgoirrt` should be linked before ASan's runtime if custom allocators are used, or after if relying on ASan's interception.
2.  **Redundancy**: Both tools may flag the same error. ASan typically traps first on basic OOB, while GOIR may trap on more complex provenance or graded-pointer violations that ASan might miss.
3.  **Symbols**: GOIR runtime symbols are prefixed with `__go_` to avoid conflicts with ASan or standard library symbols.

## Recommended Flags

```bash
clang -fsanitize=address -fgraded-optics=diag ...
```

## Testing Matrix

| Configuration | Spatial Safety | Temporal Safety | Performance Overhead |
|---|---|---|---|
| Baseline | None | None | 1.0x |
| ASan | Redzones | Quarantine | ~2.0x |
| GOIR (diag) | Graded Bounds | None (v0) | ~1.5x |
| ASan + GOIR | Combined | Combined | ~2.5x-3.0x |

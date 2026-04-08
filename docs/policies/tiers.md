# Enforcement Tiers and Policy Knobs

## Tiers
- **Diagnostic (diag)**: Instrumentation for observability.
- **Hybrid (hybrid)**: Software-based enforcement (Shadow metadata).
- **Capability (cap)**: Hardware-assisted enforcement (CHERI).

## Policy Knobs
- **Provenance**: Strict vs. Permissive.
- **Alias**: None (Standard LLVM) vs. SB-like (SoftBound-style aliasing invariants).
- **Sub-object Bounds**: Enable/Disable narrowing of bounds to struct fields.
- **Temporal Check Level**: None, Epoch-based, or Full CETS.

## OPEN-ENDED Parameters
- Toolchain: LLVM version (OPEN-ENDED, recommend 18+).
- Target Architecture: OPEN-ENDED.
- Host OS: OPEN-ENDED.

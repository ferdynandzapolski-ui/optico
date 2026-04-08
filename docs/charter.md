# GOIR Project Charter

## Goals
- Develop a Graded Optics Intermediate Representation (GOIR) for C.
- Implement spatial (bounds) and temporal (lifetime) safety for C pointers.
- Support multiple provenance policies for pointer-to-integer casts.
- Enable tiered enforcement: Diagnostic, Hybrid, and Capability-backed.
- Formalize the GOIR semantics and prove key safety properties.

## Non-Goals
- Full C language support (initial focus on a substantial subset).
- Automatic refactoring of legacy code to safer idioms.
- Replacing LLVM entirely (GOIR is designed to work with/within LLVM).

## Supported C-Subset (Initial)
- Standard integer and floating-point types.
- Structs and arrays.
- Pointers (with graded optics).
- Dynamic memory allocation (`malloc`, `free`).
- Basic control flow (`if`, `loops`, `functions`).

## UB-to-Trap Stance
- Unspecified behavior in C should be refined into deterministic traps in GOIR whenever possible.
- Spatial violations (OOB) must trap.
- Temporal violations (UAF, double-free) must trap.
- Invalid provenance resolution must trap or result in a 'TOP' (unsafe) grade.

## Policy Knobs (OPEN-ENDED)
- **Target C Dialect**: OPEN-ENDED (e.g., C99, C11, C23).
- **OS/ABI**: OPEN-ENDED (e.g., Linux/x86_64, macOS/AArch64).
- **Hardware**: OPEN-ENDED (e.g., x86_64, AArch64, CHERI/Morello).

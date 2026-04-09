# Clang Flag Integration: -fgraded-optics

## Overview
GOIR adds a new flag to the Clang driver to control the enforcement tier of graded optics. This flag is primarily used for the diagnostic prototype but is designed to support future tiers (hybrid, cap).

## Flag Syntax
`-fgraded-optics=<tier>`

### Valid Tiers:
- `diag`: Diagnostic tier. Enables basic safety checks and tracing.
- `hybrid`: Hybrid enforcement tier. Enables disjoint metadata (shadow) for pointer safety.
- `cap`: Capability-backed tier. Leverages hardware capabilities (e.g., CHERI) for enforcement.

## Internal Representation (IR)
The chosen tier is preserved in the LLVM IR as a module flag named `go-tier`.

| Tier   | `go-tier` Value |
|--------|-----------------|
| `diag` | 0               |
| `hybrid`| 1               |
| `cap`  | 2               |

## Implementation Details
1. **Frontend**: Clang driver intercepts `-fgraded-optics` and maps it to the `go-tier` module flag.
2. **Pass Plugin**: `GoInitPass` (and subsequent passes) reads the `go-tier` metadata to adjust instrumentation behavior.
3. **Runtime**: The flag ensures `libgoirrt` is linked against the final executable.

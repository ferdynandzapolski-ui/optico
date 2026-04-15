# GOIR Reference Model (Refmodel)

## Overview
The GOIR Reference Model (`refmodel`) is an executable operational semantics oracle implemented in Rust. It serves as the ground truth for GOIR instruction behavior, safety checks, and provenance policies.

## Implementation Details
- **Grade & GPtr**: Rust representations of the logical grade and graded pointer types.
- **Memory Model**:
    - A heap mapping allocation IDs to byte arrays and object metadata.
    - A shadow memory mapping addresses to grade records for pointers stored in memory.
- **Interpreter**:
    - Executes GOIR-style operations: `galloc`, `gfree`, `gload`, `gstore`, `ggep`, and `gmemcpy`.
    - Implements PNVI-AE (Provenance Not Via Integer - Address Exposure) policy for `gptrtoint` and `ginttoptr`.
- **Safety Traps**: Detects and reports `Bounds`, `Life`, `Perms`, `Prov`, and `Alias` violations.

## Provenance Policy: PNVI-AE
The refmodel implements the PNVI-AE policy:
1. `gptrtoint`: Marks the allocation associated with the pointer as "exposed".
2. `ginttoptr`: If the integer address falls within an exposed allocation, it resolves to a pointer with that allocation's provenance. Otherwise, it returns a `TOP` (untracked) grade.

## Usage
To run the reference model tests:
```bash
cd refmodel
cargo test
```

## Test Coverage
The `refmodel/tests/basic_tests.rs` suite covers:
- Basic spatial and temporal safety (OOB, UAF).
- Permission-based access control (Read-only/Write-only).
- Pointer-in-memory storage and shadow metadata propagation.
- `gmemcpy` with shadow metadata preservation and tainting.
- PNVI-AE provenance roundtrips and exposure rules.

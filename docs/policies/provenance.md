# Pointer Provenance Policies

This document defines the supported provenance policies for pointer-to-integer and integer-to-pointer casts in GOIR.

## Provenance Variants
GOIR supports at least two provenance modes:

1.  **Strict (PNVI-ae-udi like)**:
    - Pointers have an associated provenance (allocation ID).
    - `ptrtoint` exposes the provenance.
    - `inttoptr` can only resolve to a pointer with valid provenance if the integer value matches an exposed allocation.
    - Result: Precise tracking, traps on illegal forge.

2.  **Permissive (Exposed-only)**:
    - Any integer derived from an exposed pointer can be cast back to a pointer for that allocation.
    - Less precise than strict mode but more compatible with legacy idioms.

## Policy Knobs
- `GOIR_PROVENANCE_MODE`: `<Strict | Permissive>`

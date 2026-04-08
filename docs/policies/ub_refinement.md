# Undefined Behavior Refinement

GOIR aims to refine C's Undefined Behavior (UB) into well-defined traps.

## Enforcement Tiers
1.  **Diagnostic (diag)**:
    - Focus on reporting violations without necessarily stopping execution (unless configured to trap).
    - Minimal performance overhead.
    - Used for debugging and profiling.

2.  **Hybrid (hybrid)**:
    - Strong enforcement using software checks and disjoint metadata (shadow memory).
    - Traps on all spatial and temporal violations.
    - Moderate performance overhead.

3.  **Capability (cap)**:
    - Leverages hardware capability support (e.g., CHERI) for bounds and permissions.
    - Residual software checks for temporal safety and provenance.
    - High performance and strong security guarantees.

## Preservation of Behavior
- GOIR must preserve the behavior of C programs that do not exhibit UB.
- Programs that would exhibit UB in standard C will instead trap deterministically in GOIR (subject to the selected tier and policy).

# GOIR Trap Taxonomy & Bayesian Thresholds

## Safety Traps
GOIR (Graded Optics Intermediate Representation) ensures memory safety by transforming undefined behaviors into deterministic traps in enforcement tiers.

- **GO_TRAP_BOUNDS**: Spatial safety violation (out-of-bounds access).
- **GO_TRAP_LIFE**: Temporal safety violation (use-after-free).
- **GO_TRAP_PERMS**: Authorization violation (invalid permission for operation).
- **GO_TRAP_PROV**: Provenance violation (invalid pointer origin or derivation).
- **GO_TRAP_ALIAS**: Aliasing discipline violation.

## Bayesian Confidence Annotations
GOIR integrates Bayesian posteriors into the type system to gate "risky precision" optimizations.

### Invariant Beliefs
The compiler tracks posterior distributions (Beta-Binomial) for key implementation invariants:

| Invariant | Threshold (τ) | Consequence of τ Violation |
|---|---:|---|
| `inv.check_coverage` | 0.99 | Force re-verification of instrumentation coverage. |
| `inv.shadow_sync` | 0.99 | Use slower, safer shadow metadata validation. |
| `inv.memcpy_typed_ok` | 0.99 | Use byte-wise tainting instead of typed metadata lift. |
| `inv.ptrint_policy_ok` | 0.995 | Fallback to PNVI-AE (Plain-ae) conservative semantics. |
| `inv.cheri_elision_safe` | 0.995 | Keep software-based graded checks even on CHERI. |

### Soft Grades
Confidence annotations are embedded in types:
`gptr<τ> @ confidence`

A confidence of 1.0 (precise) is the default. Lower confidence values trigger the "always-safe" conservative fallbacks defined above.

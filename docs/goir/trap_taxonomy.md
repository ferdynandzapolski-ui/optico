# GOIR Trap Taxonomy & Bayesian Thresholds

## Safety Traps
GOIR (Graded Optics Intermediate Representation) ensures memory safety by transforming undefined behaviors into deterministic traps in enforcement tiers.

- **GO_TRAP_BOUNDS**: Spatial safety violation (out-of-bounds access).
- **GO_TRAP_LIFE**: Temporal safety violation (use-after-free).
- **GO_TRAP_PERMS**: Authorization violation (invalid permission for operation).
- **GO_TRAP_PROV**: Provenance violation (invalid pointer origin or derivation).
- **GO_TRAP_ALIAS**: Aliasing discipline violation.
- **GO_TRAP_UNKNOWN**: Unclassified or unexpected safety violation.

## Violation Examples & Expected Behavior

| ID | Violation Type | Description | Expected (Diag) | Expected (Hybrid) | Expected (Cap) | Law | Regression Test |
|---|---|---|---|---|---|---|---|
| 1 | `BOUNDS` | OOB read from heap array | Trap + Trace | Trap | HW Trap | GetPut | `bounds_heap_oob.c` |
| 2 | `BOUNDS` | OOB write to stack buffer | Trap + Trace | Trap | HW Trap | PutGet | `bounds_basic_oob_write.c` |
| 3 | `LIFE` | Use-after-free (load) | Trap + Trace | Trap | Residual Trap | PutGet | `lifetime_uaf_read.c` |
| 4 | `LIFE` | Double free | Trap + Trace | Trap | Residual Trap | Monotonicity | `lifetime_double_free.c` |
| 5 | `PERMS` | Write to read-only string literal | Trap + Trace | Trap | HW Trap | PutGet | `arithmetic.c` (partially) |
| 6 | `PROV` | Forged pointer from raw integer | Trap + Trace | Trap | Trap | PNVI | `prov_int_ptr_forged.c` |
| 7 | `BOUNDS` | Invalid free (non-base address) | Trap + Trace | Trap | HW Trap | Monotonicity | `bounds_interior_pointer.c` |
| 8 | `LIFE` | Access to escaped stack pointer | Trap + Trace | Trap | Trap | Monotonicity | `lifetime_stack_escape.c` |
| 9 | `BOUNDS` | OOB memcpy source | Trap + Trace | Trap | HW/SW Trap | GetPut | `memcpy_oob_src.c` |
| 10 | `PROV` | Bitwise-forged pointer deref | Trap + Trace | Trap | Trap | PNVI | `prov_int_ptr_bitwise.c` |
| 11 | `ALIAS` | Conflicting alias tag access | Trap + Trace | Trap | Trap | Alias | `lifetime_uaf_alias.c` (as proxy) |

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

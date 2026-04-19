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

## Concrete Violation Examples

The following table maps common memory safety violations to GOIR traps and expected behaviors across enforcement tiers.

| Violation Scenario | Trap Type | Diag Tier (Trap + Trace) | Hybrid Tier (Trap) | Cap Tier (HW/SW Trap) | Relevant Law | Reference Test |
|:--- |:--- |:--- |:--- |:--- |:--- |:--- |
| **Out-of-bounds Load** | `GO_TRAP_BOUNDS` | Trap with site ID & bounds | Trap | HW Bounds Fault | Monotonicity | `tests/microc/bounds_basic_oob_read.c` |
| **Use-after-free** | `GO_TRAP_LIFE` | Trap with site ID & alloc ID | Trap | SW Check Trap | PutGet (Temporal) | `tests/microc/lifetime_uaf_read.c` |
| **Double Free** | `GO_TRAP_LIFE` | Trap with site ID | Trap | SW Check Trap | Monotonicity | `tests/microc/lifetime_double_free.c` |
| **Write to Read-Only** | `GO_TRAP_PERMS` | Trap with site ID & perms | Trap | HW Perms Fault | Monotonicity | `refmodel/tests/basic_tests.rs` |
| **Free of non-base** | `GO_TRAP_BOUNDS` | Trap with site ID | Trap | HW Bounds Fault | Monotonicity | `refmodel/tests/basic_tests.rs` (test_free_non_base) |
| **Invalid ptr-int** | `GO_TRAP_PROV` | Trap with policy info | Trap | SW Check Trap | PNVI Provenance | `tests/microc/prov_ptr_int_roundtrip.c` |
| **Tainted Deref** | `GO_TRAP_PROV` | Trap with "TOP grade" log | Trap | HW/SW Trap | Monotonicity | `refmodel/tests/basic_tests.rs` |
| **OOB memcpy** | `GO_TRAP_BOUNDS` | Trap with range info | Trap | HW/SW Trap | Monotonicity | `tests/microc/memcpy_oob_src.c` |
| **One-past-end Deref** | `GO_TRAP_BOUNDS` | Trap with site ID | Trap | HW Bounds Fault | Monotonicity | `refmodel/tests/basic_tests.rs` |
| **Null Deref** | `GO_TRAP_BOUNDS` | Trap with null info | Trap | HW Fault | Monotonicity | `refmodel/tests/basic_tests.rs` (test_oob_load with galloc(0)) |
| **Alias Violation** | `GO_TRAP_ALIAS` | Trap with conflict info | Trap | SW Check Trap | Alias Invariant | `tests/microc/lifetime_uaf_alias.c` |

*Note: In the Cap tier, spatial and permission checks are typically enforced by CHERI hardware, while lifetime and provenance residuals may still require software-mediated traps.*

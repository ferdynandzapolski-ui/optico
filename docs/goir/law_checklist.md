# GOIR Law Checklist & Invariants

This document maps formal GOIR laws to their implementation points in the compiler passes and runtime library.

## Core Laws & Invariants

### 1. Graded GetPut / PutGet / PutPut
Bidirectional accessors must remain lawful under their grading constraints.

- **Enforcement Points**:
  - `GoCheckInsertPass`: Inserts `llvm.go.check_load` and `llvm.go.check_store` to ensure access is valid.
  - `runtime/goir_runtime.c`: `__go_check_load`, `__go_check_store`.
- **Temporal PutGet**: If `gstore(p, v)` succeeds and no `free(p)` occurs, `gload(p)` returns `v`.
  - `runtime/go_life.c`: Tracks `alloc_id` and `epoch` to detect intervening `free`.

### 2. Monotonicity (No Grade Forging)
Derived pointers cannot have more authority than their source.

- **Enforcement Points**:
  - `GoPropagatePass`: Ensures `ggep` only shifts bounds and preserves or restricts permissions/provenance.
  - `refmodel/src/interpreter.rs`: `ggep` implementation preserves `alloc_id` and `epoch`.
- **CHERI Lowering**: Monotonicity is hardware-guaranteed for bounds and permissions.

### 3. Join Conservativity
Control flow merges must result in a grade that is at least as restrictive as all incoming paths.

- **Enforcement Points**:
  - `GoPropagatePass`: Implements `join` (⊔) at PHI and SELECT nodes.
  - `llvm.go.join_grade` intrinsic.

### 4. PNVI Provenance Invariants
Pointer-to-integer and integer-to-pointer conversions must follow the selected PNVI policy.

- **Enforcement Points**:
  - `GoPropagatePass`: Inserts `llvm.go.prov_expose` at `ptrtoint` sites.
  - `runtime/go_prov.c`: `__go_inttoptr_resolve` ensures the resolved pointer has valid provenance.

### 5. Alias Invariants (Optional)
Aliasing discipline (e.g., Stacked Borrows-like) enforced via `G_alias`.

- **Enforcement Points**:
  - `GoCheckInsertPass`: Optionally inserts alias tracking hooks.
  - `runtime/go_alias.c`: Manages alias tokens and conflict detection.

---

## Invariant Impact by Operation

| GOIR Operation | Invariant Impact | Enforcement Mechanism |
|:--- |:--- |:--- |
| **galloc** | Establishes `base`, `end`, `alloc_id`, `epoch`, and full permissions. | `GoInitPass` (SSA), `__go_malloc` (Runtime) |
| **gfree** | Marks `alloc_id` as dead; requires `PERM_F` and base address. | `__go_check_free` |
| **gload** | Requires `[addr, addr+size) ⊆ [base, end)`, `alive`, and `PERM_R`. | `__go_check_load` |
| **gstore** | Requires `[addr, addr+size) ⊆ [base, end)`, `alive`, and `PERM_W`. Taints shadow if overlapping. | `__go_check_store`, `__go_shadow_store` |
| **ggep** | Monotonic update of `addr`. Bounds kind may change to `SUBOBJECT`. | `GoPropagatePass`, `llvm.go.gep_grade` |
| **gptrtoint** | May trigger "exposed" state for the `alloc_id` per PNVI-ae policy. | `GoPropagatePass`, `__go_prov_expose` |
| **ginttoptr** | Resolves raw address to a grade based on exposed set. | `__go_inttoptr_resolve` |
| **gmemcpy** | Propagates shadow metadata for pointer-typed fields; taints others. | `GoMemIntrinsicPass`, `__go_memcpy` |

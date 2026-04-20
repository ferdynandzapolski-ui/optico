# GOIR Law Checklist

This document provides a mapping of GOIR formal laws and invariants to their enforcement points in the compiler passes and runtime library.

## Core Laws & Invariants

### 1. Graded Optic Laws (GetPut, PutGet, PutPut)
- **GetPut**: `gload` followed by `gstore` of the same value is a no-op on the heap.
- **PutGet**: `gstore` followed by `gload` returns the stored value.
- **PutPut**: Successive `gstore` operations to the same location are equivalent to the final store.
- **Enforcement**:
    - **Compiler**: `GoCheckInsertPass` ensures checks are inserted before every load/store.
    - **Runtime**: `__go_check_load`, `__go_check_store` in `libgoirrt` verify that access is within bounds and the object is alive.

### 2. Monotonicity (No Grade Forging)
- **Law**: A derived grade `g'` must not be strictly more authoritative than its source grade `g`.
- **Enforcement**:
    - **Compiler**: `GoPropagatePass` (via `ggep_grade`) must only narrow or preserve bounds. `GoInitPass` must assign conservative initial grades.
    - **Runtime**: `__go_gep_grade` and `__go_join_grade` ensure that output grades are bounded by inputs.

### 3. Join Conservativity
- **Law**: At control-flow merges, the merged grade `g = g1 ⊔ g2` must be a safe approximation of both.
- **Enforcement**:
    - **Compiler**: `GoPropagatePass` inserts `join_grade` calls at PHI nodes and Select instructions.
    - **Runtime**: `__go_join_grade` returns a `TOP` grade if grades are incomparable, ensuring safety.

### 4. Provenance Invariants (PNVI)
- **Law**: Pointer-to-integer casts must "expose" the allocation; integer-to-pointer casts only resolve to exposed allocations.
- **Enforcement**:
    - **Compiler**: `GoProvPass` (or `GoLowerPass`) inserts `prov_expose` before `ptrtoint`.
    - **Runtime**: `__go_inttoptr_resolve` checks the global exposure set.

### 5. Alias Invariants
- **Law**: If alias discipline is enabled, pointers with conflicting tags must not access overlapping memory.
- **Enforcement**:
    - **Compiler**: Passes must preserve `alias_tok` in grades.
    - **Runtime**: Checks in `libgoirrt` (if `G_alias` is active).

---

## Invariant Impact by Operation

| Operation | Invariant Impacts & Obligations | Enforcement Point |
|---|---|---|
| `galloc` | Must create a fresh `alloc_id` and set `alive = true`. Bounds must exactly cover the requested size. | `GoInitPass`, `__go_malloc` |
| `gfree` | Must mark the object as `alive = false`. Must check that `perms.f` is set and address is `base`. | `GoCheckInsertPass`, `__go_free` |
| `gload` | Must check `bounds`, `life.alive`, and `perms.r`. Must not change the heap. | `GoCheckInsertPass`, `__go_check_load` |
| `gstore` | Must check `bounds`, `life.alive`, and `perms.w`. Must update shadow metadata if a pointer is stored. | `GoCheckInsertPass`, `__go_check_store`, `__go_shadow_store` |
| `ggep` | Must preserve `alloc_id` and `epoch`. May tighten `bounds` for sub-objects but never expand. | `GoPropagatePass`, `__go_gep_grade` |
| `gptrtoint` | Must trigger the "exposed" side-effect for the associated `alloc_id` per PNVI-AE. | `GoLowerPass`, `__go_prov_expose` |
| `ginttoptr` | Must resolve to a grade with valid `bounds` and `life` only if the address was previously exposed. | `GoLowerPass`, `__go_inttoptr_resolve` |
| `gmemcpy` | Must preserve shadow metadata for pointer-typed fields. Must taint the destination if layout is unknown. | `GoMemIntrinsicPass`, `__go_memcpy` |

# GOIR Shadow Metadata Design

This document describes the design and implementation of the disjoint shadow metadata store for pointer grades in memory.

## Objective

The shadow metadata store maps every 8-byte aligned memory location that can hold a pointer to a `go_grade_t` record. This allows GOIR to track provenance, bounds, and lifetime information for pointers stored in memory without changing the C memory layout (disjoint metadata).

## Design: Two-Level Table

To support a 48-bit address space efficiently, we use a two-level table structure.

### Parameters

- **Address Bits**: 48 bits (standard for many x86-64 and AArch64 configurations).
- **Pointer Alignment**: 8 bytes (3 bits). All pointer-grade stores/loads are assumed to be 8-byte aligned.
- **Virtual Index Bits**: $48 - 3 = 45$ bits.
- **Primary Table Bits**: 22 bits ($2^{22}$ entries).
- **Secondary Table Bits**: 23 bits ($2^{23}$ entries).
- **Grade Record Size**: 48 bytes (`go_grade_t`).

### Memory Consumption

- **Primary Table**: $2^{22}$ entries $\times$ 8 bytes/pointer = 32 MB.
- **Secondary Table**: $2^{23}$ entries $\times$ 48 bytes/record = 384 MB.

The primary table is allocated at startup using `mmap`. Secondary tables are allocated on-demand when a metadata store occurs in a new region.

### Lookup Complexity

The lookup is $O(1)$, consisting of:
1. Index calculations (shifts and masks).
2. One load from the primary table.
3. (Optional) Allocation/mapping of secondary table if not present.
4. One load/store from the secondary table.

### Index Calculation

```c
uintptr_t addr = (uintptr_t)slot_addr;
uintptr_t v_idx = addr >> 3;
uintptr_t p_idx = v_idx >> 23;
uintptr_t s_idx = v_idx & ((1ULL << 23) - 1);
```

## Thread Safety

The current implementation is **OPEN-ENDED** regarding multi-threading. In this phase, we assume single-threaded execution or external synchronization. Future iterations may use atomic pointers for the primary table and fine-grained locking or lock-free structures for secondary table allocation.

## Debug Validation

- **Alignment Checks**: `__go_shadow_store` and `__go_shadow_load` assert that `slot_addr` is 8-byte aligned.
- **Bounds Invariants**: Validation that the stored grade's bounds are consistent with the `slot_addr` (e.g., if the stored pointer must point into a specific object).

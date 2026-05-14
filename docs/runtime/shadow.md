# Disjoint Metadata (Shadow) Store Design

This document describes the design and implementation of the disjoint metadata store for GOIR pointer grades.

## Architecture

The shadow store uses a two-level table architecture to map application memory addresses to `go_grade_t` records. This design provides O(1) lookup complexity and is memory-efficient by allocating secondary tables on-demand.

### Address Mapping

We target a 48-bit virtual address space. Application pointers are assumed to be 8-byte aligned for shadow storage purposes, which allows us to ignore the lower 3 bits of the address.

The remaining 45 bits are split as follows:
- **Primary Index**: 22 bits (bits 47 to 26)
- **Secondary Index**: 23 bits (bits 25 to 3)

### Table Structure

1.  **Primary Table**: An array of pointers to Secondary Tables.
    - Size: $2^{22}$ entries.
    - Memory: $2^{22} \times 8 \text{ bytes} = 32 \text{ MiB}$.
    - Initialized at startup via a C constructor.

2.  **Secondary Table**: An array of `go_grade_t` records.
    - Size: $2^{23}$ entries.
    - Memory per table: $2^{23} \times \text{sizeof(go_grade_t)}$.
    - Allocated on-demand when a metadata store is performed in a new region.

## Implementation Details

### Allocation Strategy

- The Primary Table is allocated using `mmap` with `MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE`.
- Secondary Tables are also allocated using `mmap` with `MAP_NORESERVE`.
- Thread-safety for Secondary Table allocation is ensured using `__sync_bool_compare_and_swap` (CAS) on the Primary Table entry. If multiple threads attempt to allocate the same secondary table, only one succeeds, and the others free their redundant allocation (or we can use a more optimized approach where they wait/retry).

### Lookup Complexity

- **Store**: O(1). Requires one primary table lookup, a potential secondary table allocation, and one secondary table store.
- **Load**: O(1). Requires one primary table lookup and one secondary table load. If the secondary table is not allocated, a default "TOP" grade is returned.

### Alignment Requirements

- The `slot_addr` passed to `__go_shadow_store` and `__go_shadow_load` MUST be 8-byte aligned. This is because the shadow store maps one `go_grade_t` per 8-byte slot in application memory.

## Debug Validation

- Assertions are used to verify 8-byte alignment of slot addresses.
- An optional debug mode can be enabled to verify shadow integrity during complex operations like `memcpy`.

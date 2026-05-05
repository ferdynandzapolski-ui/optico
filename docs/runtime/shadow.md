# GOIR Shadow Metadata Design

This document describes the design of the disjoint metadata (shadow) store used in GOIR's hybrid enforcement tier. The shadow store maps memory addresses that contain pointers to their corresponding GOIR grade records.

## Architecture: Two-Level Shadow Table

To provide efficient $O(1)$ lookups while maintaining compatibility with the standard C memory layout, GOIR uses a two-level table architecture, similar to a page table or the design used by SoftBound.

### Address Space Mapping
Assuming a 48-bit virtual address space and 8-byte alignment for pointer slots:
- **Total Indexable Bits**: 45 bits (48 bits - 3 bits for alignment).
- **First-Level (Primary Table)**: 22 bits.
- **Second-Level (Secondary Tables)**: 23 bits.

### Structure
1.  **Primary Table**: A global array of pointers to secondary tables.
    -   Size: $2^{22}$ entries $\times$ 8 bytes = 32 MB.
    -   Indexed by bits [47:26] of the pointer slot address.
2.  **Secondary Tables**: Allocated on-demand.
    -   Size: $2^{23}$ entries $\times$ `sizeof(go_grade_t)`.
    -   If `go_grade_t` is 48 bytes, each table is ~384 MB.
    -   Indexed by bits [25:3] of the pointer slot address.

## Operations

### `__go_shadow_store(void* slot_addr, go_grade_t g)`
1.  Calculate indices:
    -   `idx1 = ((uintptr_t)slot_addr >> 26) & 0x3FFFFF;`
    -   `idx2 = ((uintptr_t)slot_addr >> 3) & 0x7FFFFF;`
2.  Check if `PrimaryTable[idx1]` is NULL.
3.  If NULL, allocate a new secondary table (initialized to TOP/Zero) and update `PrimaryTable[idx1]` atomically.
4.  Store `g` at `PrimaryTable[idx1][idx2]`.

### `__go_shadow_load(void* slot_addr)`
1.  Calculate indices:
    -   `idx1 = ((uintptr_t)slot_addr >> 26) & 0x3FFFFF;`
    -   `idx2 = ((uintptr_t)slot_addr >> 3) & 0x7FFFFF;`
2.  Check if `PrimaryTable[idx1]` is NULL.
3.  If NULL, return the `TOP` grade.
4.  Otherwise, return `PrimaryTable[idx1][idx2]`.

## Threading & Concurrency
-   The primary table is globally shared.
-   Allocation of secondary tables must be thread-safe (e.g., using `compare_and_swap`).
-   Individual grade updates are currently assumed to follow the same thread-safety as the pointers they describe (data races on pointers are UB in C, so races on shadow metadata are inherited).

## Memory Overhead
-   **Baseline**: 32 MB for the primary table.
-   **Dynamic**: 384 MB for every 32 GB of memory range containing at least one pointer.
-   **Optimization**: In the diagnostic tier, the shadow store can be disabled or replaced by a simpler hash map if memory is constrained.

## Debug Validation
-   In debug builds, the runtime can verify that `slot_addr` is indeed a valid, aligned memory location.
-   Shadow integrity checks can be performed to ensure that shadow metadata does not leak or become desynchronized during large memory operations like `memcpy`.

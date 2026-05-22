# Shadow Metadata Store Design

The GOIR hybrid enforcement tier requires a disjoint metadata store to track pointer grades for memory locations holding pointers. This document describes the design and implementation of the shadow store.

## Architecture

The shadow store uses a **two-level table** architecture to map application memory addresses to grade records.

- **Primary Table**: A 22-bit indexed array of pointers to secondary tables. It covers the upper bits of the 48-bit virtual address space.
- **Secondary Table**: A 23-bit indexed array of `go_grade_t` records.

### Mapping Logic

Assuming an 8-byte alignment for application pointers (pointer slots):
1. Right-shift the slot address by 3 bits to get a 45-bit index.
2. The upper 22 bits index into the Primary Table.
3. The lower 23 bits index into the Secondary Table.

This structure supports a 48-bit virtual address space ($2^{22} \times 2^{23} \times 2^3 = 2^{48}$).

## Implementation Details

- **Sparse Allocation**: The primary table and secondary tables are allocated using `mmap` with `MAP_NORESERVE`. Physical memory is only committed when pages are accessed.
- **Thread Safety**: Secondary tables are allocated on-demand using atomic compare-and-swap (`__atomic_compare_exchange_n`). This ensures that multiple threads can safely initialize different parts of the shadow store concurrently.
- **Initialization**: The primary table is initialized automatically via a C constructor (`__attribute__((constructor))`) when the runtime library is loaded.

## Metrics & Complexity

- **Lookup Complexity**: $O(1)$. A load/store involves two array lookups and one pointer dereference.
- **Memory Overhead**:
    - Primary Table: $2^{22} \times 8$ bytes = 32 MB.
    - Secondary Table: $2^{23} \times 48$ bytes (size of `go_grade_t`) = 384 MB per allocated secondary table.
    - Total overhead depends on the sparsity of pointer-heavy memory regions.

## Safety & Invariants

- **Alignment**: `__go_shadow_store` asserts that the `slot_addr` is 8-byte aligned.
- **Default Grade**: If a lookup occurs in an unallocated secondary table or at an uninitialized slot (where `base`, `end`, and `perms` are all zero), the shadow store returns a default **TOP grade** (`end = -1ULL`, `perms = 0xF`). This ensures conservative behavior and prevents false positives during diagnostic or hybrid enforcement.

## Trace Integration

The shadow store operations can be instrumented to emit trace events, though primary checks for bounds and lifetime are handled by the main safety passes.

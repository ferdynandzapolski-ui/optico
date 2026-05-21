# Disjoint Shadow Metadata Store Design

This document describes the design of the disjoint shadow metadata store for the GOIR hybrid tier. The shadow store maps every 8-byte aligned memory location that may hold a pointer to a corresponding `go_grade_t` record.

## Address Space Mapping

We target a 48-bit virtual address space. Assuming 8-byte alignment for pointer slots, there are $2^{45}$ possible pointer locations.

A two-level table structure is used to minimize memory overhead while providing O(1) lookup:

1.  **Primary Table**: Contains $2^{22}$ entries, each being a pointer to a Secondary Table.
    -   Index: bits 47-26 of the virtual address.
    -   Size: $2^{22} \times 8$ bytes = 32 MB.
2.  **Secondary Table**: Contains $2^{23}$ entries, each being a `go_grade_t` record.
    -   Index: bits 25-3 of the virtual address.
    -   Size: $2^{23} \times 48$ bytes (sizeof(go_grade_t)) = 384 MB per allocated secondary table.

### Index Calculation

```c
uintptr_t s_addr = (uintptr_t)addr >> 3;
uintptr_t primary_idx = s_addr >> 23;
uintptr_t secondary_idx = s_addr & ((1UL << 23) - 1);
```

## Memory Management

-   **Primary Table**: Allocated during runtime initialization using `mmap(NULL, 32MB, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0)`.
-   **Secondary Tables**: Allocated on-demand when a metadata store operation hits a previously unallocated region.

## Thread Safety

Secondary table allocation is made thread-safe using C11 atomic compare-and-swap:

1.  Check if `primary_table[primary_idx]` is NULL.
2.  If NULL, allocate a new secondary table using `mmap`.
3.  Use `__atomic_compare_exchange_n` to set `primary_table[primary_idx]` to the new table address.
4.  If the CAS fails (another thread won the race), `munmap` the locally allocated table and use the one already in the primary table.

## Lookup and Store API

-   `void __go_shadow_store(void* slot_addr, go_grade_t g)`: Stores a grade for the given address.
-   `go_grade_t __go_shadow_load(void* slot_addr)`: Loads the grade. Returns a default TOP grade if no metadata is present.

## Alignment Requirements

-   `slot_addr` must be 8-byte aligned. Misaligned accesses are treated as errors or return TOP.

# Shadow Metadata Design

The GOIR runtime uses a disjoint shadow metadata store to associate a `go_grade_t` record with every 8-byte word in the process's address space that might contain a pointer.

## Architecture: Two-Level Scalable Table

To support a 48-bit address space efficiently, we use a two-level table architecture inspired by page tables.

### Address Mapping

Assuming a 48-bit virtual address space and 8-byte alignment for pointer slots:

1.  **Virtual Address (48 bits):** `[Primary Index (22 bits) | Secondary Index (23 bits) | Alignment (3 bits)]`
2.  **Primary Table:** A single global array of $2^{22}$ pointers to secondary tables.
    - Size: $2^{22} \times 8 \text{ bytes} = 32 \text{ MB}$.
    - Allocated at initialization.
3.  **Secondary Table:** Allocated on-demand when a store occurs in a new region.
    - Each table contains $2^{23}$ `go_grade_t` records.
    - size: $2^{23} \times 48 \text{ bytes} = 384 \text{ MB}$ (assuming 48-byte `go_grade_t`).
    - Allocated via `mmap` with `MAP_NORESERVE`.

### Complexity

- **Lookup/Store:** $O(1)$ (two array indexing operations).
- **Space:** Sparse allocation. Only regions of memory actually used to store pointers incur shadow metadata overhead.

## Threading and Concurrency

- **Primary Table Initialization:** Done lazily or at startup.
- **Secondary Table Allocation:** Uses atomic `compare-and-swap` on primary table entries to ensure only one thread allocates a secondary table for a given region.
- **Metadata Updates:** Updates to individual `go_grade_t` records are not atomic by default. The design assumes that concurrent stores to the same memory location (and thus the same shadow slot) are already data races in the source C program.

## API

The shadow store provides the following internal API:

```c
void __go_shadow_store(void* slot_addr, go_grade_t g);
go_grade_t __go_shadow_load(void* slot_addr);
```

- `slot_addr` must be 8-byte aligned.
- `__go_shadow_load` returns a default "TOP" grade (permissive) for uninitialized or unmapped shadow regions.

## Debug Validation

- **Boundary Checks:** The shadow store can optionally verify that `slot_addr` is within valid process mappings (if platform-specific APIs are available).
- **Alignment Assertions:** `__go_shadow_store` and `__go_shadow_load` will assert that the address is 8-byte aligned in debug builds.

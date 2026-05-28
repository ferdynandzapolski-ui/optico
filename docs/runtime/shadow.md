# GOIR Hybrid Shadow Metadata Store Design

The hybrid enforcement tier uses a disjoint metadata store (shadow store) to track pointer grades for memory locations holding pointers.

## Architecture

We use a two-level page table-like structure to map the 48-bit virtual address space to pointer grades.

### Address Mapping

Application pointers are assumed to be 8-byte aligned when they store other pointers (standard for 64-bit architectures). This allows us to shift the address by 3 bits.

- **Total Address Bits**: 48
- **Primary Index**: Bits 47–26 (22 bits)
- **Secondary Index**: Bits 25–3 (23 bits)
- **Alignment Shift**: 3 bits

### Table Structures

1.  **Primary Table**:
    -   Contains $2^{22}$ entries (4,194,304 entries).
    -   Each entry is an 8-byte pointer to a secondary table.
    -   Total size: 32 MB.
    -   Allocated once at runtime initialization using `mmap` with `MAP_NORESERVE`.

2.  **Secondary Table**:
    -   Contains $2^{23}$ entries (8,388,608 entries).
    -   Each entry is a `go_grade_t` struct (48 bytes).
    -   Total size per secondary table: 384 MB.
    -   Allocated on-demand using `mmap` with `MAP_NORESERVE`.
    -   Total virtual memory reserved can be large, but actual physical memory (RSS) is only used for stored metadata.

### Concurrency

-   **Primary Table Allocation**: Initialized by a constructor.
-   **Secondary Table Allocation**: Thread-safe using atomic compare-and-swap (`__atomic_compare_exchange_n`). If multiple threads attempt to allocate the same secondary table, only one wins, and the other(s) unmap their redundant allocation.

### API

```c
void __go_shadow_store(void* slot_addr, go_grade_t g);
go_grade_t __go_shadow_load(void* slot_addr);
```

### Implementation Details

-   `slot_addr` is the address of the memory location holding a pointer.
-   `__go_shadow_store` calculates the primary and secondary indices, allocates the secondary table if necessary, and stores the grade.
-   `__go_shadow_load` calculates the indices and returns the grade if the secondary table exists; otherwise, it returns a default "TOP" grade (perms=0xF, bounds=[-1, -1]).

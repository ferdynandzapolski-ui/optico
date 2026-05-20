# Shadow Metadata Store Design

The GOIR hybrid tier uses a disjoint metadata store to track pointer grades for memory locations holding pointers. This design ensures C layout compatibility while providing O(1) metadata access.

## Architecture: Two-Level Table

To support a 48-bit virtual address space with minimal memory overhead, GOIR implements a two-level table architecture inspired by page tables.

### Address Mapping

A 64-bit application address is mapped to a shadow grade record as follows:

- **Bits 47-26 (22 bits)**: Index into the **Primary Table**.
- **Bits 25-3 (23 bits)**: Index into the **Secondary Table**.
- **Bits 2-0 (3 bits)**: Ignored (assumes 8-byte alignment for pointer slots).

```
[47 ... 26] [25 ... 3] [2 ... 0]
  Primary     Secondary    Offset (8B aligned)
  22 bits     23 bits      3 bits
```

### Table Sizes

- **Primary Table**: $2^{22}$ entries. Each entry is a pointer (8 bytes). Total size: 32 MB.
- **Secondary Table**: $2^{23}$ entries. Each entry is a `go_grade_t` (48 bytes). Total size per allocated secondary table: 384 MB.

The primary table is allocated at startup using `mmap` with `MAP_NORESERVE`. Secondary tables are allocated on-demand.

## Implementation Details

### Sparse Allocation

Secondary tables are only allocated when a grade is stored in a region for the first time. We use `mmap` with `MAP_NORESERVE` for secondary tables as well, allowing the OS to manage physical memory backing only for used pages.

### Thread Safety

The on-demand allocation of secondary tables is thread-safe using C11 atomic compare-and-swap (`__atomic_compare_exchange_n`). If multiple threads attempt to allocate the same secondary table simultaneously, only one succeeds; the others unmap their redundant allocations and use the winner's table.

### Initialization

The primary table is initialized automatically using a C constructor (`__attribute__((constructor))`).

## Complexity

- **Lookup**: O(1) - Two pointer dereferences and bitwise masking.
- **Store**: O(1) - Two pointer dereferences and bitwise masking (plus occasional on-demand allocation).
- **Space**: $O(\text{allocated regions})$. Sparse allocation ensures that only regions containing pointers consume significant memory.

## Safety & Robustness

- **Alignment**: `__go_shadow_store` asserts that the `slot_addr` is 8-byte aligned. `__go_shadow_load` returns a default **TOP grade** for unaligned addresses or uninitialized regions to ensure conservative safety.
- **Top Grade**: Uninitialized entries in an allocated secondary table are zeroed out (by `mmap`). `__go_shadow_load` interprets zeroed entries as the TOP grade (maximum bounds, all permissions).

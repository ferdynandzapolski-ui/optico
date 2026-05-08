# Shadow Metadata Design

This document describes the design of the disjoint metadata (shadow) store for GOIR pointer grades.

## Architecture: Two-Level Table

To support a 48-bit virtual address space while maintaining O(1) lookup and space efficiency, we use a two-level table architecture similar to a page table.

### Address Mapping

We assume 8-byte aligned pointer slots (3 bits ignored). A 48-bit address is split as follows:

- **Primary Index (22 bits)**: `address[47:26]`
- **Secondary Index (23 bits)**: `address[25:3]`

```
| Primary (22 bits) | Secondary (23 bits) | Ignored (3 bits) |
```

### Table Structure

1. **Primary Table**: A global array of pointers to secondary tables.
   - Size: $2^{22}$ entries $\times$ 8 bytes = 32 MB.
   - Allocated at startup via `mmap`.

2. **Secondary Table**: An array of `go_grade_t` records.
   - Size: $2^{23}$ entries $\times$ sizeof(go_grade_t).
   - Dynamically allocated via `mmap` when a primary entry is first accessed for a store.

### Lookup Algorithm

```c
go_grade_t* get_shadow_entry(void* p) {
    uintptr_t addr = (uintptr_t)p;
    uintptr_t p_idx = (addr >> 26) & 0x3FFFFF;
    uintptr_t s_idx = (addr >> 3) & 0x7FFFFF;

    if (!primary_table[p_idx]) {
        return NULL; // Or allocate for store
    }
    return &primary_table[p_idx][s_idx];
}
```

## Performance & Memory

- **Lookup Complexity**: O(1) (two array lookups).
- **Space Overhead**: Sparse allocation via `mmap` ensures we only pay for the secondary tables we use.
- **Alignment**: Pointer slots must be 8-byte aligned. Unaligned stores/loads will be handled by rounding down or trapping depending on policy.

## Thread Safety

(Open-Ended) Initial implementation assumes single-threaded or external locking. Future versions may use atomic CAS for secondary table allocation.

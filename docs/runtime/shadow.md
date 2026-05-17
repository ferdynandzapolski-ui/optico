# Shadow Metadata Store Design

The GOIR hybrid tier utilizes a disjoint shadow metadata store to map application memory locations (holding pointers) to their corresponding `go_grade_t` records.

## Architecture: Two-Level Table

To support a 48-bit virtual address space while maintaining O(1) lookup and sparse memory usage, the shadow store is implemented as a two-level table.

### Address Mapping

Application pointers are assumed to be 8-byte aligned (3-bit shift). The 48-bit virtual address is mapped as follows:

1.  **Primary Index (22 bits)**: Bits 47 through 26.
    - Used to index the **Primary Table**.
    - The Primary Table contains pointers to Secondary Tables.
    - Size: $2^{22} \times 8 \text{ bytes} = 32 \text{ MB}$.
    - Allocated upfront using `mmap` with `MAP_NORESERVE`.

2.  **Secondary Index (23 bits)**: Bits 25 through 3.
    - Used to index a **Secondary Table**.
    - Each entry in a Secondary Table is a `go_grade_t` (48 bytes).
    - Size: $2^{23} \times 48 \text{ bytes} \approx 384 \text{ MB}$.
    - Allocated on-demand when a grade is first stored in a region.

### Lookup Logic

Given an application address `addr`:
```c
uint64_t v = (uint64_t)addr;
uint64_t p_idx = (v >> 26) & 0x3FFFFF;
uint64_t s_idx = (v >> 3) & 0x7FFFFF;

secondary_table_t* st = primary_table[p_idx];
if (!st) return TOP_GRADE;
return st[s_idx];
```

## Thread Safety

Secondary table allocation is thread-safe using C11 atomic compare-and-swap (`__atomic_compare_exchange_n`). If multiple threads attempt to allocate the same secondary table simultaneously, only one will succeed, and others will use the already allocated table.

## Memory Management

- The Primary Table is allocated using `mmap` with `MAP_NORESERVE`, so physical pages are only backed when accessed.
- Secondary Tables are allocated using `mmap` only when needed.
- This design ensures that the memory overhead is proportional to the number of pointer slots used by the application, rather than the total virtual address space.

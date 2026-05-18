# Shadow Metadata Store Design

The GOIR hybrid tier utilizes a disjoint metadata store to track pointer grades for all memory locations holding pointers. This document specifies the design and implementation of this shadow store.

## Architecture

The shadow store uses a thread-safe, two-level table architecture designed to map a 48-bit virtual address space with O(1) lookup complexity.

### Address Mapping

A 48-bit application address `A` is mapped to a shadow entry using the following bit distribution:

- **Primary Index (PI)**: Bits 47–26 (22 bits)
- **Secondary Index (SI)**: Bits 25–3 (23 bits)
- **Alignment Shift**: Bits 2–0 (3 bits, assuming 8-byte alignment for pointers)

```
[ Primary Index (22) | Secondary Index (23) | Alignment (3) ]
47                 26 25                   3 2           0
```

### Table Structure

1.  **Primary Table**: A single table of 2^22 pointers (32MB). Each entry points to a Secondary Table.
2.  **Secondary Table**: A table of 2^23 `go_grade_t` records (approximately 384MB each).

The primary table is allocated at startup using `mmap` with `MAP_NORESERVE`. Secondary tables are allocated on-demand.

## Implementation Details

### Allocation

Secondary tables are allocated using `mmap`. To ensure thread safety without heavy locking, we use `__atomic_compare_exchange_n` on the primary table entries. If multiple threads attempt to allocate the same secondary table, only one will succeed, and the others will unmap their local allocations and use the successful one.

### Alignment

The shadow store enforces 8-byte alignment for application pointer slots. Storing or loading from an unaligned address is considered a programming error in the runtime and will trigger an assertion or a trap.

### Default Values

An unallocated secondary table (NULL in the primary table) or a zeroed entry in a secondary table represents the "TOP" grade (most permissive/unknown), ensuring conservative safety.

## API

- `void __go_shadow_store(void* slot_addr, go_grade_t g)`: Stores a grade for a pointer-sized slot.
- `go_grade_t __go_shadow_load(void* slot_addr)`: Loads a grade for a pointer-sized slot.

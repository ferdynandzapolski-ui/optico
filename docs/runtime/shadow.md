# Scalable Disjoint Shadow Metadata Store

The GOIR runtime uses a disjoint shadow metadata store to associate grade records (`go_grade_t`) with memory locations holding pointers. This design preserves the original layout of the program's data while providing a side-table for safety metadata.

## Mapping Scheme

To support a 48-bit virtual address space while minimizing memory overhead, the shadow store uses a two-level table hierarchy.

### Address Decomposition

A 48-bit pointer address is decomposed as follows:

- **Primary Index (22 bits)**: Bits 47 through 26.
- **Secondary Index (23 bits)**: Bits 25 through 3.
- **Ignored Offset (3 bits)**: Bits 2 through 0 (assuming 8-byte alignment).

### Table Sizes

- **Primary Table**: $2^{22}$ entries. Each entry is a pointer to a secondary table.
  - Total size: $2^{22} \times 8 \text{ bytes} = 32 \text{ MB}$.
- **Secondary Table**: $2^{23}$ entries. Each entry is a `go_grade_t` record (48 bytes).
  - Total size: $2^{23} \times 48 \text{ bytes} = 384 \text{ MB}$.

## Memory Management

Tables are allocated lazily using `mmap` with the `MAP_NORESERVE` flag. This allows the OS to overcommit virtual address space, only backing the tables with physical pages when they are actually accessed.

## Thread Safety

The shadow store is designed for concurrent access:

- The **Primary Table** is initialized atomically upon first access.
- **Secondary Tables** are allocated and installed into the primary table using atomic compare-and-swap (`atomic_compare_exchange_strong`).
- Multiple threads can safely store and load metadata concurrently. Note that individual `go_grade_t` updates are not atomic at the record level; the runtime assumes that concurrent stores to the same *shadow slot* without external synchronization are a race condition in the target program.

## Performance

Lookup complexity is $O(1)$. A load or store operation requires:
1. Two bitwise shifts and masks.
2. Two pointer dereferences (primary table, secondary table).
3. One metadata record access.

This design provides a balance between lookup speed and memory efficiency for sparse metadata distributions.

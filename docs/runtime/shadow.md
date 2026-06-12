# Shadow Metadata Store Design

The GOIR shadow metadata store provides a disjoint mapping from memory addresses holding pointers to their associated `go_grade_t` records.

## Architecture: Two-Level Table

To support a 48-bit virtual address space efficiently and sparsely, the shadow store uses a two-level table hierarchy:

1.  **Primary Table (PT)**: A 22-bit index (Bits 47–26 of the virtual address) pointing to Secondary Tables.
    - Size: 2^22 * 8 bytes = 32 MB.
2.  **Secondary Table (ST)**: A 23-bit index (Bits 25–3 of the virtual address) containing `go_grade_t` records.
    - Size: 2^23 * 48 bytes (sizeof(go_grade_t)) = 384 MB per table.
3.  **Slot Offset**: Bits 2–0 are ignored, as pointer slots are assumed to be 8-byte aligned.

## Memory Management

- **Sparse Allocation**: The PT and STs are allocated using `mmap` with the `MAP_NORESERVE` flag. This allows the OS to only commit physical pages when they are actually accessed, supporting a sparse address space with minimal physical memory overhead.
- **Lazy Initialization**: The Primary Table is initialized upon the first access to the shadow store. Secondary Tables are allocated on-demand when a store operation occurs in a new 32MB region of the address space.

## Thread Safety

- **Atomic Operations**: Allocation of the PT and STs is thread-safe using `atomic_compare_exchange_strong` on the table pointers.
- **Lock-Free Lookups**: Load operations (`__go_shadow_load`) are lock-free and use `memory_order_acquire` when reading table pointers to ensure visibility of initialized tables.
- **Store Atomicity**: Individual `go_grade_t` updates are currently non-atomic. Concurrent stores to the same slot may result in "torn" metadata if not synchronized by the application.

## Complexity

- **Lookup**: O(1) - Two pointer dereferences and bitwise masking.
- **Space**: O(N) where N is the number of 32MB regions containing pointers.

## API

```c
void __go_shadow_store(void* slot_addr, go_grade_t g);
go_grade_t __go_shadow_load(void* slot_addr);
```

## Alignment Requirements

The shadow store assumes 8-byte alignment for pointer slots. Storing to an unaligned address will currently trigger an assertion failure in debug builds. Loading from an unaligned address returns a conservative `TOP` grade.

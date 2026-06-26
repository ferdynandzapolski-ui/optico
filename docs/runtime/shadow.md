# GOIR Shadow Metadata Store

## Design Overview
The GOIR shadow metadata store is a scalable, two-level disjoint table designed to map memory slots containing pointers to their associated safety grades.

### Architecture
- **Two-Level Table**: Optimized for a 48-bit address space.
- **Primary Table**: 22 bits ($2^{22}$ entries), consuming 32MB of virtual address space.
- **Secondary Tables**: 23 bits ($2^{23}$ entries), each consuming 384MB (assuming a 48-byte `go_grade_t` record).
- **Address Mapping**:
  - Assuming 8-byte aligned pointer slots (3 bits shifted).
  - Bits 47-26: Primary Table index.
  - Bits 25-3: Secondary Table index.

### Memory Management
- **Sparse Allocation**: Uses `mmap` with `MAP_NORESERVE` to minimize physical memory overhead.
- **Lazy Initialization**: Secondary tables are allocated on-demand upon the first store/load to a specific address range.

### Performance and Threading
- **Lookup Complexity**: $O(1)$ with constant time overhead (one indirection).
- **Thread Safety**:
  - Uses atomic operations (`stdatomic.h`) for lazy allocation of table levels.
  - The `primary_table` and secondary table pointers are declared as `_Atomic`.
  - Employs `atomic_compare_exchange_strong_explicit` to ensure only one thread allocates a table level.
  - Uses `memory_order_acquire` and `memory_order_acq_rel` to ensure visibility across threads during lazy initialization.
- **Lock-Free Reads**: `__go_shadow_load` and `__go_shadow_store` are lock-free for existing entries.

## API
- `void __go_shadow_init(void)`: Initializes the primary table.
- `void __go_shadow_store(void* slot_addr, go_grade_t g)`: Stores a grade for an 8-byte aligned slot.
- `go_grade_t __go_shadow_load(void* slot_addr)`: Retrieves the grade for a slot, falling back to a `TOP` grade if uninitialized.

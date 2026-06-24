# GOIR Scalable Shadow Metadata Store

## Design Overview
The GOIR hybrid enforcement tier utilizes a disjoint shadow metadata store to map memory locations that hold pointers to their corresponding safety grades (`go_grade_t`). To preserve C memory layout compatibility, this metadata is stored in a separate address space mapping.

The implementation uses a **scalable two-level table architecture** designed for a 48-bit virtual address space.

## Architecture

### Bit Decomposition
For a 48-bit address space, assuming 8-byte alignment for pointer slots (Bits 2-0 are zero), the address is decomposed as follows:

| Range | Bits | Size | Table Level |
|---|---|---|---|
| 47-26 | 22 | 4M entries | Primary Table |
| 25-3 | 23 | 8M entries | Secondary Table |
| 2-0 | 3 | 8 bytes | Byte Offset (Ignored) |

- **Primary Table**: A single table of 4M pointers to secondary tables (~32MB).
- **Secondary Table**: Allocated on-demand. Each table holds 8M `go_grade_t` records (~384MB each, given a 48-byte grade size).

### Sparse Allocation
Both the primary and secondary tables are allocated using `mmap` with the `MAP_NORESERVE` flag. This allows the system to reserve a large virtual address space without committing physical memory until the pages are actually accessed.

### Thread Safety
Secondary table allocation is performed lazily and thread-safely using `atomic_compare_exchange_strong`. This ensures that concurrent attempts to allocate the same secondary table result in only one table being committed, while others use the already-allocated one.

## Implementation Details

- **Alignment**: Shadow operations are only supported for 8-byte aligned addresses. Unaligned accesses or accesses to uninitialized shadow regions return a conservative `TOP` grade.
- **Top Grade Synthesis**:
  - `base = 0`
  - `end = -1ULL`
  - `perms = 0xF` (Read/Write/Free/Execute)
- **Grade Size**: The `go_grade_t` struct is padded to exactly 48 bytes to ensure predictable table indexing and alignment.

## API
- `void __go_shadow_init(void)`: Initializes the global primary table.
- `void __go_shadow_store(void* slot_addr, go_grade_t g)`: Stores a grade for a pointer slot.
- `go_grade_t __go_shadow_load(void* slot_addr)`: Retrieves the grade for a pointer slot.

## Performance
- **Time Complexity**: O(1) lookup and store (two memory indirections).
- **Space Complexity**: Proportional to the number of pointer slots used in the program memory, with a 32MB fixed overhead for the primary table.

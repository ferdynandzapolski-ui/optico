# GOIR Hybrid Shadow Metadata Store

## Architecture

The GOIR shadow metadata store uses a scalable two-level disjoint table architecture to map pointer locations in the process's 48-bit address space to their associated `go_grade_t` records.

### Mapping Strategy

- **Address Bits**: 48 bits (standard user-space address range).
- **Primary Table**: 22 bits (Bits 47-26).
  - Size: 4 million entries (pointers to secondary tables).
  - Memory: 32MB (initially mapped with `MAP_NORESERVE`).
- **Secondary Table**: 23 bits (Bits 25-3).
  - Size: 8 million entries (one `go_grade_t` per 8-byte slot).
  - Memory: 384MB per table (mapped on-demand with `MAP_NORESERVE`).
  - Assumption: Metadata is stored for pointer-aligned slots (8-byte alignment).

### Thread Safety

- **Initialization**: Atomic initialization of the primary table.
- **Secondary Allocation**: Lock-free on-demand allocation of secondary tables using `atomic_compare_exchange_strong`.
- **Grade Updates**: Individual `go_grade_t` updates within a secondary table are not currently atomic (48-byte struct). Concurrent writes to the same slot may result in torn metadata if not synchronized at the source level (consistent with C memory model for non-atomic types).

### Fallbacks

- **Unmapped Ranges**: Loads from addresses that haven't been stored to (or belong to unallocated secondary tables) return a synthesized `TOP` grade.
- **Unaligned Access**: Loads from non-8-byte aligned addresses return a synthesized `TOP` grade to preserve safety at the cost of precision.

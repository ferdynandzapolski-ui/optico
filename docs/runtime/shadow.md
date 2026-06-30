# Scalable Shadow Metadata Store

The GOIR runtime uses a scalable two-level disjoint shadow metadata store to map memory locations to their corresponding safety grades.

## Design Goals

- **Scalability**: Support a 48-bit address space.
- **Performance**: O(1) lookup and store operations.
- **Memory Efficiency**: Sparse allocation of secondary tables using `mmap` with `MAP_NORESERVE`.
- **Thread Safety**: Atomic initialization of table levels.

## Address Space Mapping

The 48-bit address space is decomposed into two levels of tables, assuming 8-byte alignment for all pointer slots.

- **Bits 47-26 (22 bits)**: Index into the Primary Table.
- **Bits 25-3 (23 bits)**: Index into the Secondary Table.
- **Bits 2-0 (3 bits)**: Ignored (reserved for 8-byte alignment).

### Table Dimensions

- **Primary Table**: $2^{22}$ entries (4 million entries). Each entry is a pointer (8 bytes), resulting in a 32 MB primary table.
- **Secondary Table**: $2^{23}$ entries (8 million entries). Each entry is a `go_grade_t` record (48 bytes), resulting in a 384 MB secondary table.

Total addressable slots: $2^{45} \approx 35$ trillion 8-byte slots.

## Implementation Details

### Sparse Allocation

The Primary Table is allocated during runtime initialization. Secondary tables are allocated on-demand when a metadata store occurs in a previously unmapped region.

`mmap` with `MAP_NORESERVE` is used for both primary and secondary tables to avoid overcommitting physical memory.

### Thread Safety

The Primary Table uses atomic pointers. When multiple threads attempt to initialize the same secondary table concurrently, `compare_and_swap` ensures only one thread succeeds, while others free their redundant allocations.

### Grade Record Alignment

The `go_grade_t` struct is padded to 48 bytes to ensure efficient indexing and alignment within secondary tables.

## API

The internal shadow store provides the following interface:

- `void __go_shadow_store(void* slot_addr, go_grade_t g)`: Stores the grade `g` for the memory slot at `slot_addr`. `slot_addr` must be 8-byte aligned.
- `go_grade_t __go_shadow_load(void* slot_addr)`: Loads the grade for the memory slot at `slot_addr`. Returns a TOP grade if no metadata is present.

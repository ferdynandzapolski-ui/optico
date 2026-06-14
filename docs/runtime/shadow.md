# Disjoint Shadow Metadata Store Design

The GOIR shadow store provides a scalable, thread-safe mapping from 8-byte aligned memory addresses (holding pointers) to their associated `go_grade_t` records.

## Architecture: Two-Level Table

To support a 48-bit virtual address space while maintaining O(1) lookup and memory efficiency, we use a two-level disjoint table structure.

### Address Mapping
The address mapping is parameterized to support various virtual address space sizes. By default (48-bit):
- **Primary Index (GO_PRIMARY_BITS=22)**: `A[47:26]`
- **Secondary Index (GO_SECONDARY_BITS=23)**: `A[25:3]`
- **Offset (GO_SHIFT_BITS=3)**: `A[2:0]` (Ignored; shadow entries must be 8-byte aligned)

### Table Structures
1. **Primary Table**:
   - Size: 4,194,304 entries (2^22).
   - Content: Atomic pointers to Secondary Tables.
   - Memory: 32 MB (mapped via `mmap` with `MAP_NORESERVE`).
2. **Secondary Table**:
   - Size: 8,388,608 entries (2^23).
   - Content: `go_grade_t` records (48 bytes each).
   - Memory: 384 MB per allocated table (mapped via `mmap` with `MAP_NORESERVE`).

## Concurrency and Safety
- **Thread-Safety**: Allocation of secondary tables uses `atomic_compare_exchange_strong` to ensure only one thread initializes a table for a given primary index.
- **Atomicity**: Updates to the 48-byte `go_grade_t` records are *not* atomic. Concurrent stores to the same slot may result in torn metadata. This is consistent with the lack of synchronization on the data pointer itself in many C workloads.
- **Alignment**: The shadow store enforces 8-byte alignment. Stores to unaligned addresses are ignored, and loads return a default TOP grade.

## API
- `void __go_shadow_store(void* slot_addr, go_grade_t g)`
- `go_grade_t __go_shadow_load(void* slot_addr)`

## Performance Characteristics
- **Lookup/Store**: O(1) complexity (two pointer dereferences and an offset calculation).
- **Space**: Sparse allocation via `mmap` ensures that physical memory is only consumed for active pointer-rich regions of the heap/stack.

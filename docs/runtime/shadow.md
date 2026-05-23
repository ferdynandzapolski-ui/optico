# Disjoint Metadata (Shadow) Design

## Overview
GOIR uses a disjoint metadata store to map application pointers to their corresponding "grades" (metadata records). This design implements a scalable, two-level table architecture inspired by SoftBound and CHERI, providing O(1) lookup complexity while being memory-efficient for sparse address spaces.

## Architecture
The shadow store maps a 48-bit virtual address space. Since application pointers are assumed to be 8-byte aligned, we shift the address by 3 bits, leaving 45 bits to be mapped.

- **Primary Table (PT)**: 22 bits (4,194,304 entries).
- **Secondary Table (ST)**: 23 bits (8,388,608 entries).

### Address Translation
Given a slot address `addr`:
1. `index = addr >> 3`
2. `pt_index = index >> 23` (Bits 47-26 of the original address)
3. `st_index = index & ((1 << 23) - 1)` (Bits 25-3 of the original address)

### Memory Layout
- **Primary Table**: A single array of pointers to Secondary Tables.
  - Size: 4M entries * 8 bytes = 32 MB.
  - Allocated at startup using `mmap` with `MAP_NORESERVE`.
- **Secondary Tables**: Allocated on-demand.
  - Size: 8M entries * 48 bytes (`go_grade_t`) = 384 MB per ST.
  - Allocated using `mmap` with `MAP_NORESERVE`.

## Implementation Details

### Thread Safety
- The Primary Table is initialized once via a C constructor (`__attribute__((constructor))`).
- Allocation of Secondary Tables is thread-safe using atomic compare-and-swap (`__atomic_compare_exchange_n`).
- Updates to individual `go_grade_t` records are NOT atomic in this version. Concurrent stores to the same slot may result in torn metadata if not synchronized by the application.

### Default Grade (TOP)
If a lookup occurs in an unallocated Secondary Table or a zeroed entry, the runtime returns a default "TOP" grade:
- `base = 0`
- `end = -1ULL`
- `perms = 0xF` (Full permissions)
- All other fields zero.

## API
```c
void __go_shadow_store(void* slot_addr, go_grade_t g);
go_grade_t __go_shadow_load(void* slot_addr);
```

## Performance Stance
- **Lookup**: Two memory dereferences + bitwise ops.
- **Memory**: 32MB fixed overhead + 384MB per 64GB of used application address space (if densely packed). Sparse usage is handled efficiently by the OS via demand paging and `MAP_NORESERVE`.

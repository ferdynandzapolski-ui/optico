# GOIR Shadow Metadata Store

The hybrid tier of GOIR uses a disjoint metadata store to track grades for pointers stored in memory. This document describes the design and implementation of this store.

## Architecture

The shadow store uses a two-level table architecture to map application memory addresses to `go_grade_t` records. This design provides O(1) lookup complexity and efficient memory usage through sparse allocation.

### Address Space Mapping

We assume a 48-bit virtual address space (standard for x86_64).
Application pointers are assumed to be 8-byte aligned (3-bit shift).

The 45 bits of significant address (bits 47 to 3) are split into two levels:
- **Primary Table**: Bits 47–26 (22 bits).
- **Secondary Table**: Bits 25–3 (23 bits).

#### Index Calculation

```c
uintptr_t addr = (uintptr_t)p;
uintptr_t primary_idx = (addr >> 26) & 0x3FFFFF;
uintptr_t secondary_idx = (addr >> 3) & 0x7FFFFF;
```

### Table Structure

1.  **Primary Table**:
    - An array of $2^{22}$ pointers to secondary tables.
    - Size: $4 \times 10^6 \times 8 \text{ bytes} = 32 \text{ MB}$.
    - Allocated at startup using `mmap` with `MAP_NORESERVE`.

2.  **Secondary Table**:
    - An array of $2^{23}$ `go_grade_t` structures.
    - Each `go_grade_t` is 48 bytes (as defined in `goirrt.h`).
    - Size: $8 \times 10^6 \times 48 \text{ bytes} = 384 \text{ MB}$.
    - Allocated on-demand using `mmap` when a primary entry is first accessed.

## API

The shadow store exposes two primary functions:

```c
void __go_shadow_store(void* slot_addr, go_grade_t g);
go_grade_t __go_shadow_load(void* slot_addr);
```

### `__go_shadow_store`
Stores the grade `g` associated with the pointer stored at `slot_addr`.
If the secondary table for `slot_addr` is not yet allocated, it is allocated and initialized.

### `__go_shadow_load`
Loads the grade associated with the pointer at `slot_addr`.
If the secondary table is not allocated, it returns a `TOP` grade (conservative fallback).

## Implementation Details

- **Memory Management**: Uses `mmap` for both primary and secondary tables to leverage OS-level demand paging.
- **Thread Safety**: Current implementation is not thread-safe. Synchronization (e.g., atomic CAS for primary table entries) will be added in future iterations if needed.
- **Alignment**: The 8-byte alignment constraint is documented and checked in debug builds. Non-aligned stores will trap or degrade to `TOP`.

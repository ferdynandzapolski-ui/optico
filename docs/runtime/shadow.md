# Disjoint Shadow Metadata Design

This document describes the design and implementation of the disjoint shadow metadata store in the GOIR runtime. The shadow store maps every pointer-sized slot in the application's address space to a `go_grade_t` record.

## Architecture: Two-Level Table

To support a 48-bit address space efficiently, GOIR uses a two-level table architecture, similar to a page table.

### Address Mapping

We assume a 48-bit virtual address space. Since pointer-sized slots are 8-byte aligned, we shift the address by 3 bits to get a slot index.

- **Address Bits**: 48 bits
- **Shift**: 3 bits (8-byte alignment)
- **Primary Index**: 22 bits
- **Secondary Index**: 23 bits

Total bits accounted for: 22 + 23 + 3 = 48 bits.

### Table Structures

1.  **Primary Table**:
    - An array of $2^{22}$ pointers.
    - Each entry points to a Secondary Table.
    - Total size: $2^{22} \times 8 \text{ bytes} = 32 \text{ MB}$.
    - Allocated at startup using `mmap` with `MAP_NORESERVE`.

2.  **Secondary Table**:
    - An array of $2^{23}$ `go_grade_t` records.
    - Each `go_grade_t` is 48 bytes.
    - Total size: $2^{23} \times 48 \text{ bytes} = 384 \text{ MB}$.
    - Allocated on-demand when a store occurs in its covered address range (64 MB of application memory).

## Operations

### `__go_shadow_store(void* slot_addr, go_grade_t g)`

1.  Extract the primary index: `(addr >> 26) & 0x3FFFFF`.
2.  Check if the primary entry is NULL.
3.  If NULL, allocate a new Secondary Table using `mmap` and update the primary entry atomically.
4.  Extract the secondary index: `(addr >> 3) & 0x7FFFFF`.
5.  Store the grade `g` in the secondary table at the calculated index.

### `__go_shadow_load(void* slot_addr)`

1.  Extract the primary index.
2.  Check if the primary entry is NULL.
3.  If NULL, return a default `TOP` grade (most permissive).
4.  Extract the secondary index.
5.  Return the grade stored in the secondary table.

## Complexity

- **Time Complexity**: $O(1)$ for both load and store.
- **Space Complexity**:
    - Fixed 32 MB for the primary table.
    - 384 MB per 64 MB of application memory that contains pointers.
    - Total overhead is roughly $6 \times$ the size of application memory used for pointers.

## Memory Management

- The primary table is allocated once.
- Secondary tables are allocated on-demand and never freed during the application's lifetime to ensure $O(1)$ access without complex synchronization.
- `MAP_NORESERVE` is used to avoid pre-allocating swap space for the sparse primary table.

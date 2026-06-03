# Disjoint Metadata (Shadow) Design

This document describes the design and implementation of the disjoint metadata store for GOIR, which maps memory locations holding pointers to their corresponding grade records.

## Goals
- **O(1) Lookup**: Constant-time access to metadata for any pointer-sized slot.
- **Sparse Allocation**: Efficiently handle large, sparse address spaces without pre-allocating the entire shadow map.
- **Thread Safety**: Support concurrent stores/loads from multiple threads.
- **C Layout Compatibility**: Preserve the original C layout of the application memory.

## Two-Level Table Architecture

The shadow store uses a two-level table mapping to cover a 48-bit virtual address space.

### Address Mapping
Assuming 8-byte alignment for pointer slots (3-bit shift):
- **Bits 47-26 (22 bits)**: Primary Table Index
- **Bits 25-3 (23 bits)**: Secondary Table Index

### Data Structures
- **Primary Table**: A fixed-size array of $2^{22}$ pointers to secondary tables.
  - Size: $2^{22} \times 8$ bytes = 32 MB.
  - Allocated at startup using `mmap` with `MAP_NORESERVE`.
- **Secondary Table**: An array of $2^{23}$ `go_grade_t` records.
  - Size: $2^{23} \times 48$ bytes = 384 MB per table.
  - Allocated on-demand when a store occurs in a new 32GB region of virtual address space.

### Lookup Algorithm
1. Take the address of the pointer slot (`slot_addr`).
2. Verify 8-byte alignment: `assert((slot_addr & 7) == 0)`.
3. Extract Primary Index: `p_idx = (slot_addr >> 26) & 0x3FFFFF`.
4. Extract Secondary Index: `s_idx = (slot_addr >> 3) & 0x7FFFFF`.
5. Load Secondary Table pointer from `PrimaryTable[p_idx]`.
6. If NULL:
   - For `load`: Return TOP grade.
   - For `store`: Allocate a new Secondary Table using `mmap` and atomically update `PrimaryTable[p_idx]`.
7. Return `&SecondaryTable[s_idx]`.

## Thread Safety
- Secondary table allocation uses `atomic_compare_exchange` to ensure only one table is mapped per primary entry.
- `go_grade_t` updates are currently non-atomic (48 bytes). Concurrent stores to the same slot may result in "torn" metadata unless external synchronization is used by the application.

## Optimization
- The primary table is initialized once using a C constructor.
- Secondary tables use `MAP_NORESERVE` to avoid physical memory pressure for unused regions.

# Disjoint Metadata Shadow Store Design

## Overview
The GOIR runtime uses a disjoint metadata store (shadow store) to track safety grades for pointer values stored in memory. In the hybrid enforcement tier, this metadata is maintained in a two-level table architecture to ensure O(1) lookup and efficient memory usage.

## Two-Level Table Architecture

The shadow store maps a 48-bit virtual address space to grade records. Each 8-byte aligned application pointer slot is mapped to a `go_grade_t` record.

### Bit Mapping
The mapping uses the following bit distribution for a 48-bit address:
- **Bits [47:26] (22 bits)**: Primary Table Index.
- **Bits [25:03] (23 bits)**: Secondary Table Index.
- **Bits [02:00] (3 bits)**: Ignored (assumes 8-byte alignment).

### Table Structure
- **Primary Table**: A single table containing $2^{22}$ pointers to secondary tables. Size: 32MB.
- **Secondary Tables**: Allocated on-demand. Each table contains $2^{23}$ `go_grade_t` records. Size: $2^{23} \times 48 \text{ bytes} = 384\text{MB}$.

## Implementation Details

### Initialization
The primary table is allocated using `mmap` with `MAP_NORESERVE` during runtime initialization (via `__attribute__((constructor))`). This ensures that virtual address space is reserved but physical memory is only committed as needed.

### Thread-Safe Allocation
Secondary tables are allocated lazily when a store occurs in a new region. To handle concurrent allocations, the runtime uses atomic Compare-and-Swap (`__atomic_compare_exchange_n`) on the primary table entries. If multiple threads attempt to allocate the same secondary table, only one succeeds, and others free their redundant allocations.

### Store and Load
- `__go_shadow_store(slot_addr, grade)`: Stores a grade for a specific memory slot. Asserts 8-byte alignment.
- `__go_shadow_load(slot_addr)`: Retrieves a grade. If the slot has never been initialized or is unaligned, it returns a default `TOP` grade (full permissions, infinite bounds).

## Scalability and Performance
- **Lookup Complexity**: O(1) (two array dereferences).
- **Sparse Support**: `MAP_NORESERVE` and lazy allocation allow the shadow store to handle large, sparse address spaces without excessive physical memory overhead.

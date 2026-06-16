# Scalable Shadow Metadata Store Design

## Overview

The GOIR runtime uses a disjoint shadow metadata store to associate `go_grade_t` records with pointer slots in memory. To support large, sparse address spaces (e.g., 48-bit virtual addresses) efficiently, we implement a **two-level table architecture**.

## Architecture

The 48-bit virtual address space is mapped to shadow metadata using a hierarchical decomposition:

- **Primary Table**: A 32MB table (22 bits) containing pointers to secondary tables.
- **Secondary Table**: Each table (23 bits) contains 8.3 million `go_grade_t` records.
- **Alignment**: We assume 8-byte alignment for pointer slots (3-bit shift), matching the `GO_SHIFT_BITS` parameter.

### Bit Decomposition (Default)

| Bits | Range | Purpose |
|---|---|---|
| 47–26 | 22 bits | Primary Table Index |
| 25–3 | 23 bits | Secondary Table Index |
| 2–0 | 3 bits | Byte Offset (ignored, assuming 8-byte alignment) |

## Implementation Details

- **Sparse Allocation**: Both primary and secondary tables are allocated using `mmap` with `MAP_NORESERVE`. This ensures that physical memory is only committed when a page is actually accessed.
- **Lazy Initialization**: The primary table is initialized on the first `__go_shadow_store` or `__go_shadow_load` call. Secondary tables are allocated on-demand.
- **Thread Safety**: Table allocation is thread-safe using `atomic_compare_exchange_strong` to prevent race conditions during lazy initialization.
- **Alignment Handling**: `__go_shadow_store` ignores stores to non-8-byte aligned addresses. `__go_shadow_load` returns a conservative TOP grade for unaligned addresses or uninitialized slots.

## Complexity

- **Lookup**: O(1) – Two pointer dereferences and bitwise operations.
- **Storage**: O(N) where N is the number of active pointer slots. Sparse `mmap` keeps physical overhead minimal for typical workloads.

## Configuration

The architecture is parameterized via `#ifndef` guards in `runtime/src/go_shadow.c`, allowing it to be adapted for different address space sizes (e.g., 32-bit or 57-bit):

- `GO_PRIMARY_BITS`
- `GO_SECONDARY_BITS`
- `GO_SHIFT_BITS`

# GOIR Disjoint Shadow Metadata Store

This document describes the design and implementation of the scalable shadow metadata store used in the GOIR hybrid enforcement tier.

## Architecture

The shadow store uses a **two-level disjoint table** architecture designed to support a sparse 48-bit address space with O(1) lookup complexity.

### Address Space Mapping

The 48-bit address space is decomposed as follows, assuming pointer slots are 8-byte aligned:

- **Primary Table Index (22 bits)**: Bits 47 through 26.
- **Secondary Table Index (23 bits)**: Bits 25 through 3.
- **Alignment Bits (3 bits)**: Bits 2 through 0 (must be zero for 8-byte aligned slots).

### Table Dimensions

- **Primary Table**:
    - Entries: $2^{22}$ (approx. 4.2 million).
    - Entry Size: 8 bytes (pointer to secondary table).
    - Total Size: 32 MB.
    - Allocation: Lazily initialized upon first shadow access via `mmap` with `MAP_NORESERVE`.

- **Secondary Table**:
    - Entries: $2^{23}$ (approx. 8.4 million).
    - Entry Size: 48 bytes (`go_grade_t` struct with explicit padding).
    - Total Size: 384 MB per secondary table.
    - Allocation: Lazily allocated on-demand using `mmap` with `MAP_NORESERVE` and thread-safe atomic CAS.

## Implementation Details

### Sparse Allocation

By using `mmap` with `MAP_NORESERVE`, the runtime reserves the virtual address space for the shadow tables without immediately committing physical memory or swap space. Physical pages are only committed by the OS when the memory is first accessed (page-on-demand).

### Threading and Concurrency

- **Lazy Initialization**: The primary table and individual secondary tables are allocated on-demand.
- **Atomic Setup**: To ensure thread safety during lazy allocation without global locks, `atomic_compare_exchange_strong` (or equivalent) is used to install newly allocated secondary tables into the primary table.
- **Non-blocking Reads**: Shadow loads do not require locks. If a primary entry is NULL, it safely returns a conservative "TOP" grade.

### Alignment Requirements

The shadow store only supports metadata for 8-byte aligned memory locations. Storing or loading from unaligned addresses will:
1. Return a TOP grade (for loads).
2. Be ignored or trap (for stores, depending on configuration).

## Performance

- **Lookup Complexity**: O(1). A shadow load requires two memory dereferences (Primary -> Secondary -> Grade).
- **Store Complexity**: O(1). A shadow store requires two memory dereferences, plus a potential lazy allocation of a secondary table.

## Debugging and Validation

- **Boundary Checks**: The runtime verifies that incoming addresses are within the supported 48-bit range.
- **Invariants**: Debug builds can verify that any pointer-type store in the program is accompanied by a corresponding shadow update.

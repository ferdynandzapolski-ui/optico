# Scalable Two-Level Shadow Metadata Store

The GOIR runtime utilizes a disjoint shadow metadata store to associate safety "grades" with memory locations containing pointers. This design preserves the original memory layout of C programs while providing efficient, thread-safe access to metadata.

## Architecture

The shadow store is implemented as a two-level table hierarchy designed for a 48-bit virtual address space.

### Address Mapping

Assuming 8-byte alignment for pointer slots (3-bit shift), the 48-bit address is decomposed as follows:

| Bit Range | Size | Purpose |
| :--- | :--- | :--- |
| 47–26 | 22 bits | **Primary Table Index** |
| 25–3 | 23 bits | **Secondary Table Index** |
| 2–0 | 3 bits | Alignment (Must be zero) |

### Table Details

- **Primary Table**:
  - Size: $2^{22}$ entries.
  - Content: Atomic pointers to secondary tables.
  - Memory: ~32MB (on 64-bit systems).
  - Allocation: Lazy, via `mmap` with `MAP_NORESERVE`.

- **Secondary Tables**:
  - Size: $2^{23}$ entries per table.
  - Content: `go_grade_t` records (48 bytes each).
  - Memory: ~384MB per secondary table.
  - Allocation: On-demand, via `mmap` with `MAP_NORESERVE`.

## Memory Management

To minimize physical memory pressure, all tables are mapped using `MAP_NORESERVE`. Pages are only committed by the OS when they are actually written to. This allows the system to support a massive virtual shadow space while only consuming physical RAM for the parts of the address space actually used by the application.

## Thread Safety

- **Lazy Initialization**: The primary and secondary tables are allocated on-demand using atomic compare-and-swap (`atomic_compare_exchange_strong`).
- **Metadata Updates**: Updates to the 48-byte `go_grade_t` records are currently **non-atomic**. While this is efficient, it may lead to "torn" metadata if multiple threads perform unsynchronized stores to the same 8-byte memory slot simultaneously.

## API

- `void __go_shadow_store(void* slot_addr, go_grade_t g)`: Stores grade metadata for an 8-byte aligned slot.
- `go_grade_t __go_shadow_load(void* slot_addr)`: Retrieves grade metadata. Returns a `TOP` grade (most permissive) if the slot is uninitialized or the address is unaligned.

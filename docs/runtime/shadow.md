# GOIR Shadow Metadata Store Design

This document describes the scalable disjoint shadow metadata store used by the GOIR hybrid runtime.

## Architecture

The shadow store uses a two-level page table architecture to map 48-bit virtual addresses to 48-byte `go_grade_t` records.

### Address Decomposition

For a given 48-bit address `A`, assuming 8-byte alignment for pointer slots:

- **Primary Index (PI)**: Bits [47:26] (22 bits)
- **Secondary Index (SI)**: Bits [25:3] (23 bits)
- **Offset**: Bits [2:0] (3 bits, must be zero for valid slots)

```
47        PI        26 25        SI        3 2 Offset 0
+---------------------+---------------------+----------+
|       22 bits       |       23 bits       |  3 bits  |
+---------------------+---------------------+----------+
```

### Table Sizes

- **Primary Table (PT)**:
  - Entries: $2^{22} = 4,194,304$
  - Entry Size: 8 bytes (pointer to ST)
  - Total Size: 32 MiB
- **Secondary Table (ST)**:
  - Entries: $2^{23} = 8,388,608$
  - Entry Size: 48 bytes (`go_grade_t`)
  - Total Size: 384 MiB

## Memory Management

- The **Primary Table** is allocated at runtime initialization using `mmap` with `MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE`.
- **Secondary Tables** are allocated lazily on the first `store` operation to a slot within their range.
- `MAP_NORESERVE` is used to allow the virtual address space to be reserved without immediate physical memory or swap backing.

## Thread Safety

- **Primary Table Initialization**: Performed once via `__go_shadow_init` or lazily via atomic guards.
- **Secondary Table Allocation**: Uses atomic `compare_exchange` on PT entries to ensure only one thread allocates a given ST.
- **Metadata Updates**: Updates to 48-byte `go_grade_t` records are **non-atomic**. Concurrent stores to the same 8-byte slot may result in torn metadata. However, GOIR assumes that such data races in the target program are already UB or managed by external synchronization.
- **Metadata Reads**: Reads from unallocated STs return a default `TOP` grade (full permissions, infinite bounds).

## API

```c
void __go_shadow_store(void* slot_addr, go_grade_t g);
go_grade_t __go_shadow_load(void* slot_addr);
```

### Constraints

- `slot_addr` must be 8-byte aligned. Requests for unaligned addresses are ignored (store) or return `TOP` (load).

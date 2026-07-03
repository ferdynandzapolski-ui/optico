# Scalable Shadow Metadata Store

The GOIR runtime implements a scalable, thread-safe disjoint shadow metadata store to map pointer-sized memory slots to their corresponding grade records.

## Architecture

The shadow store uses a **two-level table hierarchy** designed to cover a 48-bit virtual address space.

### Bit Decomposition

The 48-bit address space is decomposed as follows, assuming 8-byte alignment for pointer slots:

| Bits | Width | Purpose |
|---|---|---|
| 47–26 | 22 bits | Primary Table Index |
| 25–03 | 23 bits | Secondary Table Index |
| 02–00 | 3 bits | Slot Alignment Offset (must be zero) |

- **Primary Table**: Contains 2^22 entries (4M entries). Each entry is an atomic pointer to a secondary table.
- **Secondary Table**: Contains 2^23 entries (8M entries). Each entry is a `go_grade_t` record.

### Memory Management

- **Sparse Allocation**: Both primary and secondary tables are allocated using `mmap` with `MAP_NORESERVE`. Physical memory is only backed when a page is actually accessed.
- **Lazy Initialization**: The primary table is initialized on the first `store` or `load` operation. Secondary tables are allocated on-demand when a slot in that range is first stored.

### Thread Safety

- The store uses C11 atomics (`_Atomic`, `atomic_load`, `atomic_compare_exchange_strong`) to ensure thread-safe lazy allocation of tables.
- Updates to individual `go_grade_t` records in the secondary tables are currently **not atomic**. Concurrent stores to the same slot may result in torn metadata if not synchronized by the application.

## API

The shadow store provides the following C API (see `runtime/include/go_shadow.h`):

- `void __go_shadow_init(void)`: Explicitly initialize the store.
- `void __go_shadow_store(void* slot_addr, go_grade_t g)`: Store a grade for an 8-byte aligned slot.
- `go_grade_t __go_shadow_load(void* slot_addr)`: Load a grade for a slot. Returns a `TOP` grade if uninitialized or unaligned.

## Design Parameters

The bit widths are configurable via preprocessor macros at compile time:

- `GO_PRIMARY_BITS` (default: 22)
- `GO_SECONDARY_BITS` (default: 23)
- `GO_SHIFT_BITS` (default: 3)

These defaults are chosen to balance memory overhead and lookup performance for a typical 48-bit address space.

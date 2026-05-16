# Disjoint Metadata (Shadow) Store Design

The GOIR hybrid tier utilizes a disjoint metadata store (shadow store) to map application pointers in memory to their corresponding `go_grade_t` records. This enables safety enforcement for legacy C code without altering memory layout.

## Architecture

The shadow store is implemented as a thread-safe, two-level table mapping a 48-bit virtual address space.

### Address Mapping

Application pointers are assumed to be 8-byte aligned (3-bit shift). The mapping of a 48-bit virtual address `V` is:

1.  **Primary Index**: Bits 47–26 (22 bits). Maps to a pointer to a Secondary Table.
2.  **Secondary Index**: Bits 25–3 (23 bits). Maps to a `go_grade_t` entry in the Secondary Table.

### Memory Layout

- **Primary Table**: A single array of pointers (size: $2^{22} \times 8$ bytes $\approx 32$ MB).
- **Secondary Table**: An array of `go_grade_t` records (size: $2^{23} \times 48$ bytes $\approx 384$ MB per allocated table).

### Efficiency and Scaling

- **O(1) Lookup**: Both load and store operations require constant time (two array lookups).
- **Sparse Allocation**: The Primary Table is allocated at startup using `mmap` with `MAP_NORESERVE`. Secondary Tables are allocated on-demand only when a grade is stored in a previously unallocated region.
- **Thread Safety**: Secondary table allocation uses atomic Compare-and-Swap (CAS) to ensure that concurrent stores to the same region do not leak memory or cause data races during initialization.

## API

```c
void __go_shadow_store(void* slot_addr, go_grade_t g);
go_grade_t __go_shadow_load(void* slot_addr);
```

- `slot_addr`: The address of the pointer field in application memory.
- `g`: The `go_grade_t` metadata to be associated with the pointer at `slot_addr`.

## Invariants

- A zeroed-out entry in an allocated secondary table is interpreted as a **TOP grade** (permissive default) to ensure gradual migration and compatibility.
- Accesses to `slot_addr` that fall into unallocated secondary tables return TOP grades.

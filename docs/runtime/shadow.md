# GOIR Scalable Shadow Metadata Store

## Overview
The GOIR hybrid enforcement tier requires a disjoint metadata store to track "grades" for pointers stored in memory. To support large address spaces with minimal overhead and high performance, we implement a two-level scalable shadow table.

## Address Space Mapping
We target a 48-bit virtual address space (common on x86_64 and AArch64). Since metadata is tracked for pointer-sized (8-byte) aligned slots, we only need to index $2^{45}$ possible locations.

### Two-Level Table Structure
The 45 bits of the slot index are split between a Primary Table (PT) and multiple Secondary Tables (ST):

- **Slot Index (45 bits)** = `(address >> 3)`
- **Primary Index (22 bits)** = `(Slot Index >> 23) & 0x3FFFFF`
- **Secondary Index (23 bits)** = `Slot Index & 0x7FFFFF`

### Memory Overhead
- **Primary Table**: $2^{22}$ entries * 8 bytes (pointer) = **32 MB**. This table is allocated at startup.
- **Secondary Table**: $2^{23}$ entries * sizeof(go_grade_t).
  - Assuming `go_grade_t` is padded to 48 bytes.
  - $2^{23} * 48$ bytes = **384 MB** per ST.
- **Sparse Allocation**: Secondary tables are allocated lazily using `mmap` with `MAP_NORESERVE`. Only accessed regions consume physical memory/swap space.

## Complexity
- **Lookup/Store**: $O(1)$. Specifically, two memory loads for a load, and one branch for lazy allocation on a store.
- **Space**: $O(N)$ where $N$ is the number of 8MB regions containing pointers.

## Threading
- **Lazy Initialization**: The Primary Table entries are initialized to NULL.
- **Thread Safety**: On `__go_shadow_store`, if the secondary table is NULL, it is allocated via `mmap` and installed using an atomic Compare-And-Swap (CAS). This ensures that multiple threads trying to allocate the same secondary table do not leak memory or cause data races.
- **Read-Only Access**: `__go_shadow_load` performs a lock-free load. If it sees a NULL PT entry, it returns a conservative TOP grade.

## API
```c
void __go_shadow_store(void* slot_addr, go_grade_t g);
go_grade_t __go_shadow_load(void* slot_addr);
```
Note: `slot_addr` must be 8-byte aligned. If it is not, `__go_shadow_store` ignores the request, and `__go_shadow_load` returns TOP.

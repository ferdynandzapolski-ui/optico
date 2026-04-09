# GOIR MicroC Test Suite

This directory contains deterministic C programs used to verify the end-to-end behavior of the GOIR compiler and runtime.

## Test Categories

- `bounds_*`: Spatial safety tests (Out-of-bounds access).
- `lifetime_*`: Temporal safety tests (Use-after-free, double-free).
- `prov_*`: Pointer provenance tests (ptrtoint/inttoptr).
- `memcpy_*`: Bulk memory operation tests.
- `identity.c`, `arithmetic.c`, `struct.c`: Basic sanity tests.

## Running Tests

Use the `runner.py` script to compile and run the tests:

```bash
python3 runner.py --rt-lib <path_to_libgoirrt> --rt-inc ../runtime/include <test_files>
```

Example:
```bash
python3 runner.py --rt-lib ../build/runtime --rt-inc ../runtime/include bounds_basic_oob_read.c
```

## Expected Outcomes

The `expected.json` file defines the expected behavior (PASS or TRAP) for each test case, parameterized by:
- **Enforcement Tier**: `diag`, `hybrid`, `cap`.
- **Provenance Policy**: `strict`, `permissive`.

## Trap Taxonomy

When a test traps, it should ideally report a cause code:
- `BOUNDS`: Spatial violation.
- `LIFE`: Temporal violation.
- `PERMS`: Permission violation.
- `PROV`: Provenance violation.
- `ALIAS`: Alias discipline violation.
- `UNKNOWN`: Unclassified failure.

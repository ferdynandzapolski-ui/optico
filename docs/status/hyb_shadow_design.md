# Task Status: hyb_shadow_design

## Overview
- **Status**: Completed
- **Metrics**:
  - Effort (PD): 1.5
  - Acceptance Criteria Met: Yes

## Outputs
- `runtime/include/go_shadow.h`
- `runtime/src/go_shadow.c`
- `docs/runtime/shadow.md`

## Verification Results
- **Unit Tests**: `runtime/tests/test_shadow.c` verified basic store/load, sparse allocation via `mmap`, 8-byte alignment handling, and correctly returning TOP for uninitialized slots.
- **Manual Check**: `tests/microc/manual_shadow_check.c` verified integration with `libgoirrt.so` and persistence of metadata across shadow calls.
- **MicroC Suite**: Verified no regressions in existing runtime behavior.

## Failures & Root Cause Analysis
- Initial test failed due to overly aggressive "uninitialized" heuristic in `__go_shadow_load`. Fixed by checking `base`, `end`, and `perms` together.

## Prompt Parameter Updates
- `GO_PRIMARY_BITS`, `GO_SECONDARY_BITS`, and `GO_SHIFT_BITS` are parameterized with `#ifndef` guards to support OPEN-ENDED hardware requirements.
- Uses `stdatomic.h` for thread-safe lazy initialization.
- Uses `MAP_NORESERVE` for efficient sparse memory mapping on Linux/POSIX.

## Merge Status
- Merged to main: No

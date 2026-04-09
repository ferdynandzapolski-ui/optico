# Task Status: diag_clang_flag

## Overview
- **Status**: Completed
- **Metrics**:
  - Effort (PD): 1
  - Acceptance Criteria Met: Yes

## Outputs
- `docs/dev/flags.md`
- `clang_graded_optics.patch`
- `tools/goir-clang`
- `tests/clang/graded-optics-flag.c`
- `passes/GoInitPass.cpp` (updated)
- `tests/lit.cfg.py` (updated)

## Verification Results
- `goir-clang -fgraded-optics=diag` builds and links a C program.
- IR contains `go-tier` module flag recognized by passes.
- Clang regression test `tests/clang/graded-optics-flag.c` verifies marker and linkage.
- Pass runs automatically in Clang via `PipelineStartEPCallback`.

## Failures & Root Cause Analysis
- **Issue**: Pass didn't run when using `clang -fpass-plugin`.
- **Root Cause**: The pass was only registered via `PipelineParsingCallback`, which is used by `opt -passes=...`, but Clang requires registration to an Extension Point (EP) to run automatically.
- **Fix**: Added `registerPipelineStartEPCallback` to the pass plugin.
- **Issue**: `clang -mllvm -go-tier=N` failed with "Unknown command line argument".
- **Root Cause**: Clang's `-mllvm` processing occurs before pass plugins are fully initialized in the global `cl::opt` pool.
- **Fix**: Used an environment variable `GOIR_TIER` in the wrapper script and updated the pass to read from it as a fallback.

## Prompt Parameter Updates
- None.

## Merge Status
- Merged to main: No (Pending submission)

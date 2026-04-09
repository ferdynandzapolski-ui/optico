# Task Status: diag_trace_tooling

## Overview
- **Status**: Completed
- **Metrics**:
  - Effort (PD): 0.5
  - Acceptance Criteria Met: Yes

## Outputs
- `runtime/include/go_trace.h`
- `runtime/src/go_trace.c`
- `tools/goir-trace/goir-trace.py`
- `tools/goir-trace/goir-minrepro.py`
- `docs/runtime/tracing.md`
- `docs/runtime/trace_schema.md`

## Verification Results
- **Runtime Tracing**: Implemented `__go_trace_event` and integrated it into `goir_runtime.c`.
- **Trace CLI**: `goir-trace.py` successfully summarized traces from `trace_smoke.c`.
- **MicroC Suite**: All 34 tests passed or trapped as expected using `tests/microc/runner.py`.
- **Regression**: No regressions observed in the existing MicroC suite.
- **Environment**: Manual build used for runtime due to missing LLVM/Clang CMake configs. `opt` and `lit` not found in path, so LLVM IR tests were not run.

## Failures & Root Cause Analysis
- None.

## Prompt Parameter Updates
- None.

## Merge Status
- Merged to main: No

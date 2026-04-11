# Task Status: diag_go_check_insert_pass_template

## Overview
- **Status**: Completed
- **Metrics**:
  - Effort (PD): 1.0
  - Acceptance Criteria Met: Yes

## Outputs
- `passes/GoCheckInsertPass.h`
- `passes/GoCheckInsertPass.cpp`
- `tests/llvm/go-check-insert/load_store.ll`
- Updated `passes/GoInitPass.cpp` (metadata support)
- Updated `passes/GoPropagatePass.cpp` (metadata support)
- Updated `passes/PassPlugin.cpp` (pass registration)

## Verification Results
- **Pass Implementation**: `GoCheckInsertPass` correctly instruments `load`, `store`, and `free`.
- **Metadata Propagation**: `GoInitPass` and `GoPropagatePass` now attach `!go.grade` metadata, which is used by `GoCheckInsertPass` for grade retrieval.
- **Lit Tests**: `load_store.ll` verifies check insertion order and arguments.
- **MicroC Suite**: Verified no regressions in existing tests.

## Failures & Root Cause Analysis
- Initial `lit` test had incorrect check order for `load`; fixed after code review.
- Missing `!go.grade` metadata initially limited propagation; fixed by adding metadata attachment to all relevant pointer-producing instructions.

## Prompt Parameter Updates
- None.

## Merge Status
- Merged to main: No

# Task Status: diag_go_check_insert_pass_template

## Overview
- **Status**: Completed
- **Metrics**:
  - Effort (PD): 1.0
  - Acceptance Criteria Met: Yes

## Outputs
- `passes/GoCheckInsertPass.cpp` (Enhanced with TOP-grade synthesis)
- `tests/llvm/go-check-insert/basic.ll`
- `tests/llvm/go-check-insert/remarks.ll`

## Verification Results
- **Lit Tests**: Verified that missing metadata triggers optimization remarks and conservative TOP grades.
- **Statistics**: Added `NumMissingGrades` counter.

## Failures & Root Cause Analysis
- None.

## Prompt Parameter Updates
- None.

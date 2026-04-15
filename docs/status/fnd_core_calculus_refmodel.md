# Task Status: fnd_core_calculus_refmodel

## Overview
- **Status**: Completed
- **Metrics**:
  - Effort (PD): 8
  - Acceptance Criteria Met: Yes

## Outputs
- `/refmodel/` (Rust implementation)
- `/refmodel/tests/basic_tests.rs` (20 tests)
- `/docs/goir/refmodel.md` (Documentation)

## Verification Results
- **Refmodel Tests**: 20 tests passed covering OOB, UAF, ptr-int, and memcpy.
- **Policy Verification**: PNVI-AE behavior verified for pointer-integer roundtrips.
- **Shadow Metadata**: Verified that storing a pointer to memory and loading it back preserves the grade, and that overwriting bytes taints the shadow.

## Failures & Root Cause Analysis
- Initial `cargo test` run polluted the repo with `target/` files because `.gitignore` didn't cover `/refmodel/target`. Fixed by updating `.gitignore`.

## Prompt Parameter Updates
- Toolchain: Rust (edition 2021).
- Default Provenance Policy: PNVI-AE.

## Merge Status
- Merged to main: No

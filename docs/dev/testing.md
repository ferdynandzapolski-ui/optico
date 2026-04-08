# GOIR Testing Conventions

This document defines the conventions for testing the GOIR compiler passes and runtime library.

## 1. LLVM IR Tests (lit + FileCheck)

LLVM IR tests are located in `tests/llvm/`. These tests verify that compiler passes correctly transform LLVM IR.

### Conventions
- **File extension**: `.ll`
- **Runner**: `lit` (invoked via `ninja check-goir`)
- **Verification**: `FileCheck`
- **Substitutions**:
  - `%opt`: Runs `opt` with the GOIR pass plugin loaded.

### Example Test
```llvm
; RUN: %opt -passes=go-init -S %s 2>&1 | FileCheck %s

; CHECK: GoInitPass running on module: {{.*}}
define i32 @main() {
  ret i32 0
}
```

### Remarks and Diagnostics
When a pass cannot statically prove a safety property and falls back to a conservative 'TOP' grade, it should emit an LLVM remark.
```llvm
; CHECK: remark: <input>:4:1: unknown size for malloc => TOP bounds
```

## 2. MicroC Tests (Runtime execution)

MicroC tests are small, deterministic C programs located in `tests/microc/`. These tests verify the end-to-end behavior of the compiled code and the runtime library.

### Conventions
- **File extension**: `.c`
- **Runner**: `tests/microc/runner.py`
- **Expected Outcome**: Defined in `expected.json` (to be implemented in later tasks).

### Trap Taxonomy
GOIR traps should provide a taxonomy code indicating the cause of the failure:
- `BOUNDS`: Spatial safety violation (OOB).
- `LIFE`: Temporal safety violation (UAF, double-free).
- `PERMS`: Permission violation (e.g., writing to read-only memory).
- `PROV`: Provenance violation (invalid ptr-int resolution).
- `ALIAS`: Alias discipline violation.
- `UNKNOWN`: Unclassified failure.

## 3. Policy-Aware Testing

Tests should be parameterized by the enforcement tier (`diag`, `hybrid`, `cap`) and the chosen provenance policy.
- Diagnostic tier tests should focus on trace output and trap taxonomy.
- Hybrid tier tests should verify shadow metadata propagation.
- Capability tier tests should verify correct lowering to hardware-backed operations.

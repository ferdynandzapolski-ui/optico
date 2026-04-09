# GOIR LLVM Regression Tests

This directory contains LLVM IR regression tests managed by `lit` and `FileCheck`.

## Conventions

### File Naming
- Tests should have a `.ll` extension.
- Use descriptive names reflecting the feature or bug being tested (e.g., `go-init-alloca.ll`).

### RUN Lines
- Every test must start with one or more `RUN:` lines.
- Use the `%opt` substitution for running the GOIR pass pipeline.
- Example: `; RUN: %opt -passes=go-init -S %s 2>&1 | FileCheck %s`

### FileCheck
- Use `CHECK:`, `CHECK-NEXT:`, `CHECK-LABEL:`, etc., to verify expected IR transformations.
- Capture the output of `opt` (including stderr if remarks are used) and pipe it to `FileCheck`.

### Remarks
- When testing conservative fallbacks or precision loss, use `CHECK:` to verify that appropriate LLVM remarks are emitted.
- Example: `; CHECK: remark: <input>:4:1: unknown size for malloc => TOP bounds`

### Directory Structure
- Subdirectories may be used to group tests by pass (e.g., `go-init/`, `go-propagate/`).

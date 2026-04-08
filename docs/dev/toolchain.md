# Toolchain Pinning Strategy

## LLVM/Clang
- **Baseline**: LLVM 18.1.3
- **Pinning**: In CI, use a specific container image with the pinned LLVM version.
- **Upstream Drift**: If a build breaks due to upstream drift, update this document and lock the new version.

## CMake
- **Minimum Version**: 3.24

## Parameterized Targets
- The toolchain is designed to be target-agnostic. Target triple and hardware features are passed as parameters to the build system.

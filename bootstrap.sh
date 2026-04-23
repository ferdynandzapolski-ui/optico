#!/bin/bash
# bootstrap.sh - Bootstrap the minimal self-hosted OptiCo compiler

set -e

echo "=============================================="
echo "OptiCo Minimal Compiler Bootstrap"
echo "=============================================="

# Build the Rust compiler
echo "[1/4] Building Rust compiler..."
source $HOME/.cargo/env 2>/dev/null || true
cargo build --release

# Compile the minimal .oco compiler to LLVM IR
echo "[2/4] Compiling minimal compiler to LLVM IR..."
./target/release/app minimal_compiler.oco

# Compile LLVM IR to object file
echo "[3/4] Compiling LLVM IR to object file..."
llc -filetype=obj minimal_compiler.ll -o minimal_compiler.o

# Link to executable
echo "[4/4] Linking to executable..."
clang main.c minimal_compiler.o -o optico_minimal

echo ""
echo "=============================================="
echo "Bootstrap complete!"
echo "=============================================="
echo "Generated files:"
echo "  minimal_compiler.ll  - LLVM IR"
echo "  minimal_compiler.o   - Object file"
echo "  optico_minimal       - Executable"
echo ""
echo "Test the minimal compiler:"
echo "  echo '1+2' | ./optico_minimal"
echo "  (Should print: 3)"
echo ""
echo "Test self-hosting:"
echo "  ./optico_minimal minimal_compiler.oco"
echo "  (Should recompile itself)"
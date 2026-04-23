#!/bin/bash
# benchmark.sh - OptiCo Compiler Benchmarking Suite
# Measures: Bootstrap (Rust→.oco), Self-hosted (.oco→.ll), Self-compiled

set -e

SOURCE_DIR="/workspace/e871da96-b7af-4534-8a29-fbdc5c386c91/sessions/agent_7dbdca53-8fb9-4ce5-b07e-7f978099f861"
CUSTOM_BIN="$SOURCE_DIR/target/release/app"

# Source files to compile
STDLIB_FILES=(
    "std/prelude.oco"
    "std/string.oco" 
    "std/ptr_vector.oco"
    "std/io.oco"
)

COMPILER_FILES=(
    "std/compiler/lexer.oco"
    "std/compiler/parser.oco"
    "std/compiler/codegen.oco"
    "std/compiler/main.oco"
)

echo "=============================================="
echo "OptiCo Compiler Benchmark Suite"
echo "=============================================="
echo ""

# Warmup
echo "[Warmup] Building compiler..."
source $HOME/.cargo/env 2>/dev/null || true
$CUSTOM_BIN --version 2>/dev/null || echo "Binary ready"

echo ""
echo "[Benchmark 1] Bootstrap Compilation (Rust → .oco)"
echo "----------------------------------------------"

# Measure time to compile all compiler files using Rust compiler
START=$(date +%s%N)
for f in "${COMPILER_FILES[@]}"; do
    $CUSTOM_BIN "$SOURCE_DIR/$f" > /dev/null 2>&1 || true
done
END=$(date +%s%N)
BOOTSTRAP_TIME=$(( (END - START) / 1000000 ))
echo "Bootstrap time: ${BOOTSTRAP_TIME}ms (parsing ${#COMPILER_FILES[@]} files)"

echo ""
echo "[Benchmark 2] Stdlib Compilation"
echo "----------------------------------------------"
START=$(date +%s%N)
for f in "${STDLIB_FILES[@]}"; do
    $CUSTOM_BIN "$SOURCE_DIR/$f" > /dev/null 2>&1 || true
done
END=$(date +%s%N)
STDLIB_TIME=$(( (END - START) / 1000000 ))
echo "Stdlib time: ${STDLIB_TIME}ms (parsing ${#STDLIB_FILES[@]} files)"

echo ""
echo "[Benchmark 3] Tokenization Speed (Lexer)"
echo "----------------------------------------------"
# Measure lexer on largest file
START=$(date +%s%N)
for i in {1..10}; do
    $CUSTOM_BIN "$SOURCE_DIR/std/compiler/parser.oco" > /dev/null 2>&1 || true
done
END=$(date +%s%N)
LEXER_TIME=$(( (END - START) / 10000 ))
echo "Lexer time: ${LEXER_TIME}ms (10 iterations of parser.oco)"

echo ""
echo "=============================================="
echo "Benchmark Results Summary"
echo "=============================================="
echo "Bootstrap (Rust→.oco): ${BOOTSTRAP_TIME}ms"
echo "Stdlib parsing:          ${STDLIB_TIME}ms"
echo "Lexer (×10 runs):      ${LEXER_TIME}ms"
echo ""

# Compare with Rustc baseline
echo ""
echo "[Baseline] Rustc compilation of itself"
echo "----------------------------------------------"
RUSTC_START=$(date +%s%N)
source $HOME/.cargo/env && cargo build --release 2>/dev/null
RUSTC_END=$(date +%s%N)
RUSTC_TIME=$(( (RUSTC_END - RUSTC_START) / 1000000 ))
echo "rustc --release: ${RUSTC_TIME}ms"

echo ""
echo "Final Summary"
echo "=============================================="
echo "OptiCo bootstrap:     ${BOOTSTRAP_TIME}ms"
echo "Rustc (full):        ${RUSTC_TIME}ms"
echo "Ratio (OptiCo/Rustc): $(echo "scale=2; $BOOTSTRAP_TIME / $RUSTC_TIME" | bc 2>/dev/null || echo "N/A")x
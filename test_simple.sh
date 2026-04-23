#!/bin/bash
# test_simple.sh - Test the simple OptiCo program

set -e

echo "=============================================="
echo "Testing Simple OptiCo Program"
echo "=============================================="

# Build the Rust compiler
echo "[1/4] Building Rust compiler..."
source $HOME/.cargo/env 2>/dev/null || true
cargo build --release

# Compile the simple test to LLVM IR
echo "[2/4] Compiling simple_test.oco to LLVM IR..."
./target/release/app simple_test.oco

# Rename main to optico_main in LLVM IR to avoid conflict
sed -i 's/@main/@optico_main/g' simple_test.ll

# Compile LLVM IR to object file
echo "[3/5] Compiling LLVM IR to object file..."
llc -filetype=obj simple_test.ll -o simple_test.o

# Create a simple main.c that calls the OptiCo main
echo "[4/6] Creating main.c wrapper..."
cat > main.c << 'EOF'
#include <stdio.h>

void print_int(int val) {
    printf("%d\n", val);
}

extern void optico_main();

int main() {
    optico_main();
    return 0;
}
EOF

# Link to executable
echo "[5/6] Linking to executable..."
clang main.c simple_test.o -o simple_test_exe

echo ""
echo "=============================================="
echo "Test complete!"
echo "=============================================="
echo "Generated files:"
echo "  simple_test.ll    - LLVM IR"
echo "  simple_test.o     - Object file"
echo "  simple_test_exe   - Executable"
echo "  main.c            - C wrapper"
echo ""
echo "Running test:"
echo "  ./simple_test_exe"
echo "  (Should print: 3)"
echo ""

# Run the test
echo "Running ./simple_test_exe:"
./simple_test_exe
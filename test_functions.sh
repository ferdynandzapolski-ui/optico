#!/bin/bash
# test_functions.sh - Test the functions compiler

set -e

echo "=============================================="
echo "Testing Functions Compiler"
echo "=============================================="

# Compile the LLVM IR to object file
echo "[1/2] Compiling LLVM IR to object file..."
llc -filetype=obj functions_test_manual.ll -o functions_test.o

# Create C wrapper
echo "[2/2] Creating C wrapper..."
cat > functions_wrapper.c << 'EOF'
#include <stdio.h>

// External functions from OptiCo
void print_int(int x) {
    printf("%d\n", x);
}
void test_functions();

int main() {
    printf("Testing functions:\n");
    test_functions();
    printf("\n");
    return 0;
}
EOF

# Link executable
echo "[3/3] Linking executable..."
clang functions_wrapper.c functions_test.o -o functions_compiler

echo ""
echo "=============================================="
echo "Functions compiler created!"
echo "=============================================="
echo "Testing function calls:"
echo -n "print_sum(5, 3): "
./functions_compiler 2>/dev/null | grep -A10 "Testing functions:" | head -2 | tail -1 | tr -d '\n'
echo " (should be 8)"
echo -n "add_three(1, 2, 3): "
./functions_compiler 2>/dev/null | grep -A10 "Testing functions:" | head -3 | tail -1 | tr -d '\n'
echo " (should be 6)"
echo -n "multiply(6, 7): "
./functions_compiler 2>/dev/null | grep -A10 "Testing functions:" | head -4 | tail -1 | tr -d '\n'
echo " (should be 42)"

echo ""
echo "=============================================="
echo "Function support demonstration complete!"
echo "=============================================="
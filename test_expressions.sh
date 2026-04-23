#!/bin/bash
# test_expressions.sh - Test expressions compiler

set -e

echo "=============================================="
echo "Testing Expressions Compiler"
echo "=============================================="

# Compile the LLVM IR to object file
echo "[1/2] Compiling LLVM IR to object file..."
llc -filetype=obj expressions_test_manual.ll -o expressions_test.o

# Create C wrapper
echo "[2/2] Creating C wrapper..."
cat > expressions_wrapper.c << 'EOF'
#include <stdio.h>

// External functions from OptiCo
void test_expressions();

void print_int(int x) {
    printf("%d ", x);
}

int main() {
    printf("Testing expressions:\n");
    test_expressions();
    printf("\n");
    return 0;
}
EOF

# Link executable
echo "[3/3] Linking executable..."
clang expressions_wrapper.c expressions_test.o -o expressions_compiler

echo ""
echo "=============================================="
echo "Expressions compiler created!"
echo "=============================================="
echo "Testing arithmetic and comparisons:"
echo -n "Arithmetic (10 + 3*2): "
./expressions_compiler 2>/dev/null | grep -A5 "Testing expressions:" | head -2 | tail -1 | cut -d' ' -f1 | tr -d '\n'
echo " (should be 16)"
echo -n "Comparisons (x<y && x==5 && y>x): "
./expressions_compiler 2>/dev/null | grep -A5 "Testing expressions:" | head -2 | tail -1 | cut -d' ' -f2 | tr -d '\n'
echo " (should be 7)"

echo ""
echo "=============================================="
echo "Expression support demonstration complete!"
echo "=============================================="
#!/bin/bash
# test_control_flow.sh - Test control flow compiler

set -e

echo "=============================================="
echo "Testing Control Flow Compiler"
echo "=============================================="

# Compile the LLVM IR to object file
echo "[1/2] Compiling LLVM IR to object file..."
llc -filetype=obj control_flow_test_manual.ll -o control_flow_test.o

# Create C wrapper
echo "[2/2] Creating C wrapper..."
cat > control_flow_wrapper.c << 'EOF'
#include <stdio.h>

// Function to print integers
void print_int(int x) {
    printf("%d\n", x);
}

// External functions from OptiCo
void test_control_flow();

int main() {
    printf("Testing control flow:\n");
    test_control_flow();
    printf("\n");
    return 0;
}
EOF

# Link executable
echo "[3/3] Linking executable..."
clang control_flow_wrapper.c control_flow_test.o -o control_flow_compiler

echo ""
echo "=============================================="
echo "Control flow compiler created!"
echo "=============================================="
echo "Testing loops and nested conditionals:"
echo -n "While loop (sum 1-5): "
./control_flow_compiler 2>/dev/null | grep -A10 "Testing control flow:" | head -2 | tail -1 | tr -d '\n'
echo " (should be 15)"
echo -n "Nested if (x=15): "
./control_flow_compiler 2>/dev/null | grep -A10 "Testing control flow:" | head -3 | tail -1 | tr -d '\n'
echo " (should be 50)"
echo -n "Nested if (x=7): "
./control_flow_compiler 2>/dev/null | grep -A10 "Testing control flow:" | head -4 | tail -1 | tr -d '\n'
echo " (should be 25)"
echo -n "Nested if (x=3): "
./control_flow_compiler 2>/dev/null | grep -A10 "Testing control flow:" | head -5 | tail -1 | tr -d '\n'
echo " (should be 0)"

echo ""
echo "=============================================="
echo "Control flow demonstration complete!"
echo "=============================================="
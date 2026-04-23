#!/bin/bash
# test_vars.sh - Test the variables compiler

set -e

echo "=============================================="
echo "Testing Variables Compiler"
echo "=============================================="

# Compile the LLVM IR to object file
echo "[1/3] Compiling LLVM IR to object file..."
llc -filetype=obj simple_vars_compiler_manual.ll -o simple_vars_compiler.o

# Create C wrapper
echo "[2/3] Creating C wrapper..."
cat > vars_wrapper.c << 'EOF'
#include <stdio.h>
#include <string.h>

// External print function
void print_int(int x) {
    printf("%d\n", x);
}

// External functions from OptiCo
void test_variables();
int compile(char* source);

int main(int argc, char* argv[]) {
    if (argc == 1) {
        // Test variables
        printf("Testing variables:\n");
        test_variables();
        printf("\n");
    } else if (argc == 2) {
        // Compile expression
        char* expr = argv[1];
        printf("Compiling expression: %s\n", expr);
        int result = compile(expr);
        printf("Result: %d\n", result);
    } else {
        printf("Usage: %s [expression]\n", argv[0]);
        printf("  No args: test variables\n");
        printf("  With arg: compile expression\n");
        return 1;
    }
    return 0;
}
EOF

# Link executable
echo "[3/3] Linking executable..."
clang vars_wrapper.c simple_vars_compiler.o -o vars_compiler

echo ""
echo "=============================================="
echo "Variables compiler created!"
echo "=============================================="
echo "Files created:"
echo "  simple_vars_compiler_manual.ll - LLVM IR with variables"
echo "  simple_vars_compiler.o         - Compiled object file"
echo "  vars_wrapper.c                 - C wrapper"
echo "  vars_compiler                  - Executable"
echo ""
echo "Testing variable declarations:"
echo -n "Variables test: "
./vars_compiler 2>/dev/null | grep -A1 "Testing variables:" | tail -1 | tr -d '\n'
echo " (should be 30)"
echo ""

echo "Testing expression compilation:"
echo -n "1+2 = "
./vars_compiler "1+2" 2>/dev/null | grep "Result:" | cut -d' ' -f2
echo -n "10+20+5 = "
./vars_compiler "10+20+5" 2>/dev/null | grep "Result:" | cut -d' ' -f2

echo ""
echo "=============================================="
echo "Variable support demonstration complete!"
echo "=============================================="
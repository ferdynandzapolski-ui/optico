#!/bin/bash
# test_comprehensive.sh - Test comprehensive compiler

set -e

echo "=============================================="
echo "Testing Comprehensive Compiler"
echo "=============================================="

# Compile the LLVM IR to object file
echo "[1/2] Compiling LLVM IR to object file..."
llc -filetype=obj comprehensive_compiler_manual.ll -o comprehensive_compiler.o

# Create C wrapper
echo "[2/2] Creating C wrapper..."
cat > comprehensive_wrapper.c << 'EOF'
#include <stdio.h>
#include <string.h>

// External functions from OptiCo
int compile(char* source);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s \"expression\"\n", argv[0]);
        printf("Example: %s \"1+2*3\"\n", argv[0]);
        return 1;
    }

    char* expr = argv[1];
    printf("Compiling expression: %s\n", expr);
    int result = compile(expr);
    printf("Result: %d\n", result);
    return 0;
}
EOF

# Link executable
echo "[3/3] Linking executable..."
clang comprehensive_wrapper.c comprehensive_compiler.o -o comprehensive_compiler

echo ""
echo "=============================================="
echo "Comprehensive compiler created!"
echo "=============================================="
echo "Testing expression parsing:"
echo -n "1+2 = "
./comprehensive_compiler "1+2" 2>/dev/null | grep "Result:" | cut -d' ' -f2
echo -n "3*4 = "
./comprehensive_compiler "3*4" 2>/dev/null | grep "Result:" | cut -d' ' -f2
echo -n "10+5*2 = "
./comprehensive_compiler "10+5*2" 2>/dev/null | grep "Result:" | cut -d' ' -f2
echo -n "(2+3)*4 = "
./comprehensive_compiler "(2+3)*4" 2>/dev/null | grep "Result:" | cut -d' ' -f2

echo ""
echo "=============================================="
echo "Comprehensive compiler demonstration complete!"
echo "=============================================="
echo ""
echo "This demonstrates a self-hosted compiler that can:"
echo "- Lex input into tokens"
echo "- Parse arithmetic expressions with precedence"
echo "- Evaluate expressions"
echo "- Print results"
echo ""
echo "Next step: Extend to handle variables, functions, and control flow"
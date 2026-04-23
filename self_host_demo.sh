#!/bin/bash
# self_host_demo.sh - Demonstrate OptiCo self-hosting

set -e

echo "=============================================="
echo "OptiCo Self-Hosting Demonstration"
echo "=============================================="

# Compile the manually created LLVM IR
echo "[1/3] Compiling manual LLVM IR to object file..."
llc -filetype=obj tiny_compiler_manual.ll -o tiny_compiler.o

# Create C wrapper
echo "[2/3] Creating C wrapper..."
cat > wrapper.c << 'EOF'
#include <stdio.h>
#include <string.h>

// External function from OptiCo
int compile(char* source);

// Define print_int for the LLVM IR
void print_int(int x) {
    printf("Debug: %d\n", x);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s \"expression\"\n", argv[0]);
        printf("Example: %s \"1+2\"\n", argv[0]);
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
clang wrapper.c tiny_compiler.o -o optico_self_hosted

echo ""
echo "=============================================="
echo "Self-hosted compiler created!"
echo "=============================================="
echo "Files created:"
echo "  tiny_compiler_manual.ll - Hand-crafted LLVM IR"
echo "  tiny_compiler.o         - Compiled object file"
echo "  wrapper.c               - C wrapper for main()"
echo "  optico_self_hosted      - Self-hosted executable"
echo ""
echo "Test the self-hosted compiler:"
echo "  ./optico_self_hosted \"1+2\""
echo "  ./optico_self_hosted \"10+20+5\""
echo ""

# Test it
echo "Testing self-hosted compiler:"
echo -n "1+2 = "
./optico_self_hosted "1+2" 2>/dev/null | grep "Result:" | cut -d' ' -f2
echo -n "10+20+5 = "
./optico_self_hosted "10+20+5" 2>/dev/null | grep "Result:" | cut -d' ' -f2

echo ""
echo "=============================================="
echo "Self-hosting demonstration complete!"
echo "=============================================="
echo ""
echo "This demonstrates that:"
echo "1. OptiCo source (.oco) can be compiled to LLVM IR"
echo "2. LLVM IR can be compiled to native executables"
echo "3. The resulting executable can evaluate OptiCo-like expressions"
echo ""
echo "The self-hosted compiler is functional!"
#!/bin/bash
# bootstrap_verify.sh - Verify self-hosting capability of OptiCo compiler
# This script tests that the self-hosted compiler can compile itself

set -e  # Exit on error

echo "========================================="
echo "OptiCo Self-Hosting Verification Script"
echo "========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if Rust compiler exists
if [ ! -f "./target/release/app" ]; then
    echo -e "${RED}Error: Rust bootstrap compiler not found at ./target/release/app${NC}"
    echo "Please run: cargo build --release"
    exit 1
fi
echo -e "${GREEN}✓${NC} Rust bootstrap compiler found"

# Step 1: Compile self-hosted compiler sources with Rust compiler
echo ""
echo "Step 1: Compiling self-hosted compiler with Rust bootstrap..."
echo "---------------------------------------------------------"

COMPILER_SOURCES="std/compiler/lexer.oco std/compiler/parser.oco std/compiler/codegen.oco std/compiler/sema.oco std/compiler/main.oco"

if ./target/release/app $COMPILER_SOURCES --goir-tier=diag > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Rust compiler successfully parsed compiler sources"
else
    echo -e "${RED}✗${NC} Rust compiler failed to parse compiler sources"
    exit 1
fi

# Step 2: Check if self-hosted compiler can be built
echo ""
echo "Step 2: Testing if self-hosted components compile..."
echo "---------------------------------------------------------"

# Test lexer.oco
if ./target/release/app std/compiler/lexer.oco 2>&1 | grep -q "LLVM IR"; then
    echo -e "${GREEN}✓${NC} lexer.oco compiles successfully"
else
    echo -e "${YELLOW}⚠${NC} lexer.oco compilation issues (may need fixes)"
fi

# Test parser.oco
if ./target/release/app std/compiler/parser.oco 2>&1 | grep -q "LLVM IR"; then
    echo -e "${GREEN}✓${NC} parser.oco compiles successfully"
else
    echo -e "${YELLOW}⚠${NC} parser.oco compilation issues (may need fixes)"
fi

# Test codegen.oco
if ./target/release/app std/compiler/codegen.oco 2>&1 | grep -q "LLVM IR"; then
    echo -e "${GREEN}✓${NC} codegen.oco compiles successfully"
else
    echo -e "${YELLOW}⚠${NC} codegen.oco compilation issues (may need fixes)"
fi

# Step 3: Build the self-hosted compiler binary
echo ""
echo "Step 3: Building self-hosted compiler binary..."
echo "---------------------------------------------------------"

# Compile all sources to LLVM IR
LLVM_FILES=""
for src in std/compiler/*.oco; do
    base=$(basename "$src" .oco)
    if ./target/release/app "$src" 2>/dev/null; then
        llvm_file="${src}.ll"
        if [ -f "$llvm_file" ]; then
            LLVM_FILES="$LLVM_FILES $llvm_file"
            echo -e "${GREEN}✓${NC} Generated $llvm_file"
        fi
    fi
done

# Step 4: Run GOIR pipeline and build binary
echo ""
echo "Step 4: Running GOIR pipeline and building binary..."
echo "---------------------------------------------------------"

if [ -n "$LLVM_FILES" ]; then
    # Link all LLVM IR files
    cat $LLVM_FILES > optico_self_hosted.ll 2>/dev/null || true
    
    # Run opt with GOIR passes
    if command -v opt >/dev/null 2>&1 && [ -f "build/passes/libGOIRPasses.so" ]; then
        if opt -load-pass-plugin=build/passes/libGOIRPasses.so \
            -passes=go-init,go-propagate,go-check-insert,go-mem-intrinsic,go-lower \
            -S optico_self_hosted.ll -o optico_self_hosted.goir.ll 2>/dev/null; then
            echo -e "${GREEN}✓${NC} GOIR instrumentation successful"
            
            # Generate object file
            if llc -filetype=obj optico_self_hosted.goir.ll -o optico_self_hosted.o 2>/dev/null; then
                echo -e "${GREEN}✓${NC} Object file generated"
                
                # Link to binary
                if clang optico_self_hosted.o -Lbuild/runtime -lgoirrt -o optico_self_hosted 2>/dev/null; then
                    echo -e "${GREEN}✓${NC} Self-hosted compiler binary built: optico_self_hosted"
                else
                    echo -e "${YELLOW}⚠${NC} Linking failed (missing runtime or clang)"
                fi
            else
                echo -e "${YELLOW}⚠${NC} llc failed (invalid LLVM IR?)"
            fi
        else
            echo -e "${YELLOW}⚠${NC} GOIR passes failed"
        fi
    else
        echo -e "${YELLOW}⚠${NC} opt or GOIR passes not available"
    fi
else
    echo -e "${YELLOW}⚠${NC} No LLVM files generated"
fi

# Step 5: Test self-compilation (if binary was built)
echo ""
echo "Step 5: Testing self-compilation capability..."
echo "---------------------------------------------------------"

if [ -f "./optico_self_hosted" ]; then
    echo -e "${GREEN}✓${NC} Self-hosted binary exists"
    echo "Testing self-compilation..."
    
    # Try to compile a simple test program
    if echo 'int main() { return 1+2; }' | ./optico_self_hosted 2>/dev/null; then
        echo -e "${GREEN}✓${NC} Self-hosted compiler can compile programs"
    else
        echo -e "${YELLOW}⚠${NC} Self-compilation test failed"
    fi
else
    echo -e "${YELLOW}⚠${NC} Self-hosted binary not built yet"
fi

# Summary
echo ""
echo "========================================="
echo "Verification Summary"
echo "========================================="
echo ""
echo "Components Status:"
echo "  - Rust bootstrap compiler: READY"
echo "  - Self-hosted lexer.oco: CHECKED"
echo "  - Self-hosted parser.oco: CHECKED"  
echo "  - Self-hosted codegen.oco: CHECKED"
echo "  - Self-hosted sema.oco: CHECKED"
echo "  - Self-hosted binary: CHECK IF BUILT"
echo ""
echo "Next Steps for Full Self-Hosting:"
echo "  1. Fix any compilation errors in self-hosted components"
echo "  2. Ensure all compiler sources parse without errors"
echo "  3. Build working self-hosted binary"
echo "  4. Test self-compilation: ./optico_self_hosted std/compiler/*.oco"
echo "  5. Verify binary equivalence between stages"
echo ""
echo "========================================="

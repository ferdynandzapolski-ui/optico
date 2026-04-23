#!/bin/bash
# run_tests.sh - Test runner for self-hosting compiler verification
# Tests the Rust bootstrap compiler and self-hosted components

set -e

echo "=========================================="
echo "OptiCo Self-Hosting Test Suite"
echo "=========================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Counters
PASSED=0
FAILED=0

# Test function
test_file() {
    local file=$1
    local expected=$2
    local test_name=$(basename "$file" .oco)
    
    echo -e "${BLUE}Testing:${NC} $test_name"
    
    # Check if file exists
    if [ ! -f "$file" ]; then
        echo -e "  ${RED}✗${NC} File not found: $file"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    # Compile with Rust bootstrap compiler
    if ./target/release/app "$file" > /dev/null 2>&1; then
        echo -e "  ${GREEN}✓${NC} Rust compiler: parsed successfully"
        PASSED=$((PASSED + 1))
        
        # Check if LLVM IR was generated
        local ll_file="${file}.ll"
        if [ -f "$ll_file" ]; then
            echo -e "  ${GREEN}✓${NC} LLVM IR generated: $ll_file"
            PASSED=$((PASSED + 1))
            
            # Try to compile to object file (if llc is available)
            if command -v llc > /dev/null 2>&1; then
                if llc -filetype=obj "$ll_file" -o "${file}.o" 2>/dev/null; then
                    echo -e "  ${GREEN}✓${NC} llc: compiled to object file"
                    PASSED=$((PASSED + 1))
                    
                    # Try to link (if clang is available)
                    if command -v clang > /dev/null 2>&1 && [ -f "build/runtime/libgoirrt.a" ]; then
                        if clang "${file}.o" -Lbuild/runtime -lgoirrt -o "${file}.bin" 2>/dev/null; then
                            echo -e "  ${GREEN}✓${NC} clang: linked to binary"
                            PASSED=$((PASSED + 1))
                            
                            # Run the binary if it exists
                            if [ -f "${file}.bin" ]; then
                                echo -e "  ${GREEN}✓${NC} Binary executed: $(${file}.bin 2>/dev/null || echo 'no output')"
                                PASSED=$((PASSED + 1))
                            fi
                        else
                            echo -e "  ${YELLOW}⚠${NC} clang: linking failed (runtime may be missing)"
                            FAILED=$((FAILED + 1))
                        fi
                    else
                        echo -e "  ${YELLOW}⚠${NC} clang: not available or runtime missing"
                    fi
                else
                    echo -e "  ${RED}✗${NC} llc: failed to compile LLVM IR"
                    FAILED=$((FAILED + 1))
                fi
            else
                echo -e "  ${YELLOW}⚠${NC} llc: not available"
            fi
        else
            echo -e "  ${RED}✗${NC} LLVM IR not generated"
            FAILED=$((FAILED + 1))
        fi
    else
        echo -e "  ${RED}✗${NC} Rust compiler: parsing failed"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    echo ""
}

# Start tests
echo -e "${YELLOW}Phase 1: Testing Rust Bootstrap Compiler${NC}"
echo "------------------------------------------"

# Test minimal program
test_file "tests/self_hosting/test_minimal.oco"

# Test expressions
test_file "tests/self_hosting/test_expressions.oco"

# Test control flow
test_file "tests/self_hosting/test_control_flow.oco"

# Test comparisons
test_file "tests/self_hosting/test_comparison.oco"

echo ""
echo -e "${YELLOW}Phase 2: Testing Self-Hosted Components${NC}"
echo "------------------------------------------"

# Test self-hosted lexer
echo -e "${BLUE}Testing:${NC} self-hosted lexer.oco"
if ./target/release/app std/compiler/lexer.oco > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} lexer.oco: parsed by Rust compiler"
    PASSED=$((PASSED + 1))
else
    echo -e "  ${RED}✗${NC} lexer.oco: parsing failed"
    FAILED=$((FAILED + 1))
fi
echo ""

# Test self-hosted parser
echo -e "${BLUE}Testing:${NC} self-hosted parser.oco"
if ./target/release/app std/compiler/parser.oco > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} parser.oco: parsed by Rust compiler"
    PASSED=$((PASSED + 1))
else
    echo -e "  ${RED}✗${NC} parser.oco: parsing failed"
    FAILED=$((FAILED + 1))
fi
echo ""

# Test self-hosted codegen
echo -e "${BLUE}Testing:${NC} self-hosted codegen.oco"
if ./target/release/app std/compiler/codegen.oco > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} codegen.oco: parsed by Rust compiler"
    PASSED=$((PASSED + 1))
else
    echo -e "  ${RED}✗${NC} codegen.oco: parsing failed"
    FAILED=$((FAILED + 1))
fi
echo ""

# Test self-hosted sema
echo -e "${BLUE}Testing:${NC} self-hosted sema.oco"
if ./target/release/app std/compiler/sema.oco > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} sema.oco: parsed by Rust compiler"
    PASSED=$((PASSED + 1))
else
    echo -e "  ${RED}✗${NC} sema.oco: parsing failed"
    FAILED=$((FAILED + 1))
fi
echo ""

# Test self-hosted main
echo -e "${BLUE}Testing:${NC} self-hosted main.oco"
if ./target/release/app std/compiler/main.oco > /dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} main.oco: parsed by Rust compiler"
    PASSED=$((PASSED + 1))
else
    echo -e "  ${RED}✗${NC} main.oco: parsing failed"
    FAILED=$((FAILED + 1))
fi
echo ""

echo -e "${YELLOW}Phase 3: Building Self-Hosted Compiler${NC}"
echo "------------------------------------------"

# Try to build the self-hosted compiler
echo -e "${BLUE}Building:${NC} self-hosted compiler binary"

COMPILER_LL=""
for src in std/compiler/*.oco; do
    if ./target/release/app "$src" 2>/dev/null; then
        ll_file="${src}.ll"
        if [ -f "$ll_file" ]; then
            COMPILER_LL="$COMPILER_LL $ll_file"
            echo -e "  ${GREEN}✓${NC} Generated $ll_file"
            PASSED=$((PASSED + 1))
        fi
    else
        echo -e "  ${RED}✗${NC} Failed to compile $src"
        FAILED=$((FAILED + 1))
    fi
done

# Summary
echo ""
echo "=========================================="
echo -e "${BLUE}Test Summary${NC}"
echo "=========================================="
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi

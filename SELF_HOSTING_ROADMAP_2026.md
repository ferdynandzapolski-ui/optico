# OptiCo Self-Hosting Compiler Roadmap 2026

## Executive Summary

**Mission**: Build a self-hosting OptiCo compiler that can compile its own complete source code (lexer.oco + parser.oco + codegen.oco + main.oco) into a native binary executable, which can then recompile itself producing a bit-identical or functionally equivalent binary.

**Current Status (April 2026)**: 
- ✅ Rust bootstrap compiler fully functional (parses, semantically analyzes, generates LLVM IR)
- ✅ Self-hosted compiler components exist (lexer.oco, parser.oco, codegen.oco, main.oco)
- ⚠️ Self-hosted components are incomplete and cannot yet compile the full compiler
- ⚠️ Codegen.oco produces incomplete/invalid LLVM IR
- ⚠️ Parser.oco cannot handle all language constructs needed to parse compiler sources

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    BOOTSTRAP PROCESS                           │
├─────────────────────────────────────────────────────────────────┤
│  Stage 0: Rust Compiler (app)                                │
│  - Reads .oco files                                          │
│  - Parses → AST → Semantic Analysis → LLVM IR               │
│  - Output: .ll files                                         │
├─────────────────────────────────────────────────────────────────┤
│  Stage 1: Compile Self-Hosted Compiler Source                │
│  - Rust compiler compiles:                                    │
│    std/compiler/lexer.oco                                    │
│    std/compiler/parser.oco                                   │
│    std/compiler/codegen.oco                                  │
│    std/compiler/main.oco                                      │
│  - Output: compiler.ll → compiler.o → optico_bin (binary)    │
├─────────────────────────────────────────────────────────────────┤
│  Stage 2: Self-Compilation (The Verification)                │
│  - ./optico_bin std/compiler/*.oco                           │
│  - Output: compiler_self.ll → compiler_self.o → optico_bin_v2 │
├─────────────────────────────────────────────────────────────────┤
│  Stage 3: Verification                                       │
│  - Compare optico_bin and optico_bin_v2                      │
│  - They should be functionally equivalent (same output)       │
│  - Ideally bit-identical (or within acceptable differences)  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Current Codebase Analysis

### Rust Bootstrap Compiler (src/*.rs) - ✅ COMPLETE
| Component | File | Lines | Status |
|-----------|------|-------|--------|
| Lexer | lexer.rs | 270 | ✅ Complete |
| Parser | parser.rs | 942 | ✅ Complete |
| AST | ast.rs | 101 | ✅ Complete |
| Semantic Analysis | sema.rs | 916 | ✅ Complete |
| CIR Lowering | cir.rs | 101 | ✅ Complete |
| Code Generation | codegen.rs | 453 | ✅ Complete (AST→LLVM IR) |
| Main Driver | main.rs | 129 | ✅ Complete |
| Persistence | persistence.rs | 201 | ✅ Complete |

**Total**: 3,273 lines of Rust - fully functional bootstrap compiler.

### Self-Hosted Compiler (std/compiler/*.oco) - ⚠️ INCOMPLETE

| Component | File | Lines | Status | Issues |
|-----------|------|-------|--------|--------|
| Lexer | lexer.oco | 11,398 | ⚠️ Partial | Needs to match Rust lexer.rs features |
| Parser | parser.oco | 12,281 | ⚠️ Partial | Cannot parse all OptiCo constructs |
| Codegen | codegen.oco | 8,374 | ⚠️ Partial | Produces invalid LLVM IR |
| Main | main.oco | 4,192 | ⚠️ Partial | Missing file I/O, argument parsing |

**Total**: ~36,245 lines of OptiCo - exists but non-functional.

### Standard Library (std/*.oco) - ⚠️ PARTIAL

| Component | Status | Description |
|-----------|--------|-------------|
| prelude.oco | ✅ Good | Core types (Pair, Option, Result, String) |
| string.oco | ✅ Good | String operations |
| vector.oco | ⚠️ Basic | Integer vector, needs more features |
| ptr_vector.oco | ⚠️ Basic | Pointer vector |
| list.oco | ⚠️ Basic | Linked list |
| map.oco | ⚠️ Basic | Map/dictionary |
| io.oco | ⚠️ Basic | print_int, print_char only |
| net.oco | ❌ Stub | Networking primitives |
| ir/node.oco | ✅ Good | IR node definitions |
| ir/symbol.oco | ✅ Good | Symbol table |
| ir/type.oco | ✅ Good | Type representation |

---

## Development Phases

### Phase 1: Complete Self-Hosted Lexer ✅ PARTIALLY DONE
**Goal**: Self-hosted lexer.oco must tokenize identically to Rust lexer.rs

**Tasks**:
- [x] Basic token types (keywords, identifiers, literals)
- [x] String literal parsing with escape sequences
- [x] Comment handling (// and /* */)
- [ ] **FIX**: Match all token types from Rust lexer.rs
- [ ] **FIX**: Handle all OptiCo v0.3 keywords
- [ ] **FIX**: Proper error reporting
- [ ] **TEST**: Lexer produces identical tokens for same input

**Files to Modify**: `std/compiler/lexer.oco`

---

### Phase 2: Complete Self-Hosted Parser ⚠️ CRITICAL GAP
**Goal**: Parser.oco must parse all OptiCo constructs that the compiler itself uses

**Tasks**:
- [x] Basic expression parsing (arithmetic, comparison)
- [x] Function declarations and calls
- [x] Struct definitions
- [ ] **FIX**: Complete operator precedence parsing
- [ ] **FIX**: Parse all statement types (if-else, while, blocks)
- [ ] **FIX**: Handle variable declarations with initialization
- [ ] **FIX**: Parse extern "C" blocks correctly
- [ ] **FIX**: Parse resource declarations
- [ ] **FIX**: Parse protocol definitions
- [ ] **FIX**: Handle all type constructs (pointers, optics, contexts)
- [ ] **FIX**: Build complete AST with proper node types
- [ ] **TEST**: Parser can parse ALL compiler source files

**Files to Modify**: `std/compiler/parser.oco`

---

### Phase 3: Complete Self-Hosted Code Generation ⚠️ CRITICAL GAP
**Goal**: Codegen.oco must produce valid LLVM IR for all constructs

**Tasks**:
- [x] Basic LLVM IR generation framework
- [x] Simple expression code generation
- [ ] **FIX**: Generate valid LLVM IR syntax (currently broken)
- [ ] **FIX**: Function definition generation
- [ ] **FIX**: Variable allocation (alloca) and access (load/store)
- [ ] **FIX**: Control flow (br, label instructions)
- [ ] **FIX**: Struct type definitions and GEP instructions
- [ ] **FIX**: Function calls with arguments
- [ ] **FIX**: Return instruction generation
- [ ] **IMPLEMENT**: String constants and global data
- [ ] **IMPLEMENT**: External function declarations
- [ ] **TEST**: Generated IR compiles with llc → native object file

**Files to Modify**: `std/compiler/codegen.oco`

---

### Phase 4: Semantic Analysis in Self-Hosted Compiler 🆕 NEW
**Goal**: Add type checking and semantic analysis to self-hosted compiler

**Tasks**:
- [ ] Symbol table management
- [ ] Type checking for expressions
- [ ] Type checking for statements
- [ ] Function signature validation
- [ ] Variable scope resolution
- [ ] Resource linearity checking (SSFG)
- [ ] Comonadic type verification
- [ ] Error reporting

**Files to Create**: `std/compiler/sema.oco`

---

### Phase 5: Standard Library Enhancement ⚠️ NEEDED
**Goal**: Extend std lib to support compiler implementation

**Tasks**:
- [ ] Complete file I/O operations (read_file, write_file)
- [ ] Enhance string operations (needed by compiler)
- [ ] Enhance vector/map operations (symbol tables)
- [ ] Add memory allocation utilities
- [ ] Add debugging/diagnostic utilities

**Files to Modify**: `std/*.oco`

---

### Phase 6: Build System and Bootstrap Script 🆕 NEW
**Goal**: Create robust build system for self-hosting verification

**Tasks**:
- [ ] Update bootstrap.sh for new compiler structure
- [ ] Create test script for self-compilation
- [ ] Add binary comparison/verification
- [ ] Create CI pipeline for self-hosting tests
- [ ] Document build process

**Files to Create/Modify**: `bootstrap.sh`, `verify_self_hosting.sh`

---

### Phase 7: Self-Hosting Verification ✅ FINAL GOAL
**Goal**: Demonstrate complete self-hosting

**Verification Steps**:
1. [ ] Rust compiler compiles self-hosted compiler sources → optico_bin
2. [ ] optico_bin compiles its own sources → optico_bin_v2
3. [ ] optico_bin_v2 compiles sources → optico_bin_v3
4. [ ] Verify: optico_bin and optico_bin_v2 are functionally equivalent
5. [ ] Verify: optico_bin_v2 and optico_bin_v3 are functionally equivalent
6. [ ] Test: All produce same output for same input
7. [ ] Benchmark: Compilation speed comparison

---

## Implementation Strategy

### Approach: Fix and Extend Self-Hosted Components

Given the large existing codebase in std/compiler/*.oco, the strategy is:

1. **Audit**: Compare Rust compiler output with self-hosted compiler for same input
2. **Fix**: Correct bugs in self-hosted lexer, parser, codegen
3. **Extend**: Add missing features to match Rust compiler capabilities
4. **Test**: Continuously verify with bootstrap comparison
5. **Document**: Update all docs with actual status

### Priority Order

1. **HIGH**: Fix codegen.oco to produce valid LLVM IR (blocking everything)
2. **HIGH**: Fix parser.oco to handle all OptiCo constructs
3. **HIGH**: Complete lexer.oco to match Rust lexer
4. **MEDIUM**: Add semantic analysis (sema.oco)
5. **MEDIUM**: Enhance standard library
6. **LOW**: Optimize and polish

---

## Testing Strategy

### Test Categories

1. **Unit Tests**: Individual functions in lexer, parser, codegen
2. **Integration Tests**: Full compilation pipeline
3. **Bootstrap Tests**: Self-compilation verification
4. **Equivalence Tests**: Compare Rust vs Self-hosted output
5. **Regression Tests**: Ensure fixes don't break working features

### Test Files Needed

```
tests/
├── self_hosting/
│   ├── test_lexer.oco          # Test lexer components
│   ├── test_parser.oco         # Test parser components
│   ├── test_codegen.oco        # Test code generation
│   ├── test_bootstrap.oco      # Test bootstrap process
│   └── verify_equivalence.sh   # Compare outputs
└── examples/
    ├── minimal.oco             # Smallest compilable program
    ├── calculator.oco           # Basic arithmetic
    ├── fibonacci.oco           # Functions and recursion
    └── compiler_parts.oco      # Pieces of compiler for testing
```

---

## Success Criteria

### Minimum Viable Self-Hosting (MVSH)

- [ ] Self-hosted compiler compiles its own lexer.oco without errors
- [ ] Self-hosted compiler compiles its own parser.oco without errors
- [ ] Self-hosted compiler compiles its own codegen.oco without errors
- [ ] Self-hosted compiler compiles its own main.oco without errors
- [ ] Linking all produces optico_bin that runs
- [ ] optico_bin can compile a simple test program (e.g., 1+2)

### Full Self-Hosting Verification

- [ ] optico_bin can compile ALL compiler sources (full self-compilation)
- [ ] Resulting binary (optico_bin_v2) can recompile sources again
- [ ] All three stages produce functionally equivalent binaries
- [ ] Test suite passes at all stages
- [ ] Performance is acceptable (< 5x slowdown vs Rust bootstrap)

### Binary Equivalence (Stretch Goal)

- [ ] optico_bin and optico_bin_v2 are bit-identical (same hash)
- [ ] Or: They produce identical output for all test inputs
- [ ] Deterministic compilation achieved

---

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Self-hosted codegen too broken to fix | High | Gradually port Rust codegen.rs to OptiCo |
| Parser cannot handle complex constructs | High | Use Rust parser output as reference, match exactly |
| LLVM IR generation too complex | Medium | Start with minimal IR, add features incrementally |
| Standard library insufficient | Medium | Enhance std lib in parallel with compiler work |
| Bootstrap verification fails | High | Extensive testing at each step, keep Rust compiler as reference |
| Performance unacceptable | Low | Optimize after correctness achieved |

---

## Timeline Estimate

| Phase | Duration | Dependencies | Milestone |
|-------|----------|--------------|-----------|
| Phase 1: Lexer | 1 week | None | Lexer matches Rust version |
| Phase 2: Parser | 2 weeks | Phase 1 | Parser handles all compiler sources |
| Phase 3: Codegen | 3 weeks | Phase 2 | Valid LLVM IR generated |
| Phase 4: Semantics | 2 weeks | Phase 3 | Type checking working |
| Phase 5: Std Lib | 1 week | None | Compiler has needed utilities |
| Phase 6: Build | 1 week | Phase 4,5 | Bootstrap script works |
| Phase 7: Verify | 1 week | Phase 6 | Self-hosting demonstrated |
| **Total** | **9 weeks** | | **Complete self-hosting** |

---

## Immediate Next Steps (Week 1)

1. **Audit lexer.oco vs lexer.rs**
   - List all token types in Rust lexer
   - Check which are missing in OptiCo lexer
   - Fix discrepancies

2. **Audit parser.oco vs parser.rs**
   - List all parse functions in Rust parser
   - Check which constructs are not handled in OptiCo
   - Create fix list

3. **Audit codegen.oco vs codegen.rs**
   - Understand what valid LLVM IR looks like
   - Fix codegen to produce valid IR syntax
   - Test with simple programs

4. **Create test infrastructure**
   - Simple test programs
   - Comparison scripts
   - Bootstrap verification script

---

## Documentation Updates Needed

- [ ] Update `SELF_HOSTING_STATUS.md` with actual status
- [ ] Update `IMPLEMENTATION_SUMMARY.md` to reflect reality
- [ ] Create `docs/self_hosting_guide.md` with step-by-step instructions
- [ ] Update `README.md` with current capabilities
- [ ] Create `CHANGELOG.md` to track progress

---

## Conclusion

The OptiCo project has a solid foundation with a working Rust bootstrap compiler and substantial self-hosted compiler code. The primary challenge is fixing and completing the self-hosted components to achieve functional self-hosting.

**Key Insight**: The self-hosted compiler code exists (36K+ lines) but is non-functional. Rather than rewriting from scratch, we should systematically fix and extend the existing code to match the Rust compiler's capabilities.

**Success Metric**: When `optico_bin` (compiled by Rust) can successfully compile all `std/compiler/*.oco` files and produce a working `optico_bin_v2` that can do the same, self-hosting is achieved.

---

*Roadmap Version: 2.0 (April 2026)*
*Next Review: Weekly during implementation*

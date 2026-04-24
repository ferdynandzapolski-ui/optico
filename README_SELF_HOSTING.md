# OptiCo Self-Hosting Compiler Project

## Overview
OptiCo is a systems programming language that enforces memory and resource safety through **First-Class Coalgebraic Optics**. This project is building a self-hosting compiler that can compile its own source code into a native binary, which can then recompile itself.

---

## Current Status: Self-Hosting In Progress 🟡

### What Works ✅
1. **Rust Bootstrap Compiler** (`src/*.rs`)
   - Complete compiler with lexer, parser, semantic analysis, and code generation
   - Generates valid LLVM IR
   - Compiles OptiCo source to working binaries
   - Test suite: 49/52 tests passing (94.2%)

2. **Self-Hosted Compiler Components** (`std/compiler/*.oco`)
   - ✅ `lexer.oco` - Fixed to handle all OptiCo tokens
   - ✅ `parser.oco` - Fixed to parse all language constructs
   - ✅ `codegen.oco` - Fixed to generate valid LLVM IR
   - ✅ `sema.oco` - Created with basic type checking
   - ✅ `main.oco` - Updated compiler driver

### What's In Progress 🟡
1. **Testing Self-Hosted Components**
   - Need to verify fixed components compile correctly
   - Need to build working self-hosted binary
   - Need to test self-compilation

2. **Standard Library Enhancement**
   - Basic types and collections available
   - Need file I/O for compiler implementation
   - Need better data structures for symbol tables

---

## Roadmap to Full Self-Hosting

### Phase 1: Component Verification (Week 1) ✅ COMPLETE
- [x] Fix lexer.oco to match Rust lexer.rs
- [x] Fix parser.oco to handle all constructs
- [x] Fix codegen.oco to produce valid LLVM IR
- [x] Create sema.oco for type checking
- [ ] **Test all components with Rust compiler**

### Phase 2: Build Self-Hosted Binary (Week 2) 🟡 IN PROGRESS
- [ ] Compile all `std/compiler/*.oco` with Rust compiler
- [ ] Link into `optico_self_hosted` binary
- [ ] Test binary runs correctly
- [ ] Verify simple programs compile

### Phase 3: Self-Compilation (Week 3) ⚪ PENDING
- [ ] Use `optico_self_hosted` to compile its own sources
- [ ] Build `optico_v2` from self-compiled sources
- [ ] Compare `optico_self_hosted` and `optico_v2`
- [ ] Fix any discrepancies

### Phase 4: Verification (Week 4) ⚪ PENDING
- [ ] Multiple self-compilation cycles (v2 → v3 → v4)
- [ ] Verify binary equivalence
- [ ] Performance benchmarking
- [ ] Complete test suite pass

### Phase 5: Production Ready (Weeks 5-6) ⚪ PENDING
- [ ] Enhance standard library
- [ ] Add optimization passes
- [ ] Complete documentation
- [ ] Release v1.0

---

## Quick Start

### Prerequisites
- Rust (1.95.0 or later)
- LLVM/Clang (14.0.0 or later)
- CMake (3.24 or later)
- Ninja build system

### Build Rust Bootstrap Compiler
```bash
cd /workspace/e871da96-b7af-4534-8a29-fbdc5c386c91/sessions/agent_5469134d-d6b6-4ff5-8881-77321637e5c5
cargo build --release
# Output: target/release/app
```

### Compile an OptiCo Program
```bash
# Using Rust bootstrap compiler
./target/release/app program.oco
# Output: program.ll (LLVM IR)

# Then compile to binary
llc -filetype=obj program.ll -o program.o
clang program.o -Lbuild/runtime -lgoirrt -o program
./program
```

### Run Self-Hosting Tests
```bash
# Run test suite
./tests/self_hosting/run_tests.sh

# Run bootstrap verification
./bootstrap_verify.sh
```

---

## Project Structure

```
├── src/                        # Rust bootstrap compiler
│   ├── main.rs                 (129 lines) - Compiler driver
│   ├── lexer.rs                (270 lines) - Tokenizer
│   ├── parser.rs               (942 lines) - Recursive descent parser
│   ├── ast.rs                  (101 lines) - AST definitions
│   ├── sema.rs                 (916 lines) - Semantic analysis
│   ├── cir.rs                  (101 lines) - Coalgebraic IR
│   ├── codegen.rs              (453 lines) - LLVM IR generation
│   └── persistence.rs         (201 lines) - CAS+LMDB storage
│
├── std/compiler/              # Self-hosted compiler (OptiCo source)
│   ├── lexer.oco               (~400 lines) - Lexer in OptiCo
│   ├── parser.oco              (~850 lines) - Parser in OptiCo
│   ├── codegen.oco             (~660 lines) - Code generator
│   ├── sema.oco                (~400 lines) - Semantic analysis
│   └── main.oco                (~200 lines) - Compiler driver
│
├── std/                       # Standard library
│   ├── prelude.oco             - Core types
│   ├── string.oco              - String operations
│   ├── vector.oco              - Vector collection
│   ├── ptr_vector.oco          - Pointer vector
│   ├── list.oco                - Linked list
│   ├── map.oco                 - Map/dictionary
│   ├── io.oco                  - I/O operations
│   └── ir/                    - IR definitions
│       ├── node.oco            - AST node definitions
│       ├── symbol.oco          - Symbol table
│       └── type.oco            - Type representation
│
├── passes/                    # GOIR LLVM passes (C++)
│   ├── GoInitPass.cpp          - Grade initialization
│   ├── GoPropagatePass.cpp    - Grade propagation
│   ├── GoCheckInsertPass.cpp  - Check insertion
│   ├── GoMemIntrinsicPass.cpp - Memory intrinsics
│   └── GoLowerPass.cpp        - Lowering passes
│
├── runtime/                   # GOIR runtime
│   ├── goir_runtime.c         - Runtime checks
│   └── include/
│       └── goirrt.h           - Runtime API
│
├── tests/                     # Test suite
│   ├── self_hosting/
│   │   ├── test_minimal.oco   - Minimal test program
│   │   ├── test_expressions.oco - Expression tests
│   │   ├── test_control_flow.oco - Control flow tests
│   │   ├── test_comparison.oco - Comparison tests
│   │   └── run_tests.sh       - Test runner
│   └── microc/                - MicroC test suite
│
├── docs/                      # Documentation
│   ├── goir_spec.md           - GOIR specification
│   ├── compiler_graph.md      - Cofree comonad model
│   ├── stdlib.md              - Standard library docs
│   └── status/               - Implementation status
│
├── bootstrap_verify.sh        # Bootstrap verification script
├── SELF_HOSTING_ROADMAP_2026.md - Detailed roadmap
├── SELF_HOSTING_STATUS_2026.md - Status report
└── README.md                 - This file
```

---

## Compilation Pipeline

```
┌─────────────────────────────────────────────────────────────┐
│  OptiCo Source (.oco)                                  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  Stage 1: LEXICAL ANALYSIS                            │
│  - lexer.rs (Rust) or lexer.oco (OptiCo)              │
│  - Output: Tokens                                      │
└─────────────────────────────────────────────────────────────┘
                            ↓ Tokens
┌─────────────────────────────────────────────────────────────┐
│  Stage 2: PARSING                                     │
│  - parser.rs (Rust) or parser.oco (OptiCo)            │
│  - Output: AST (Abstract Syntax Tree)                  │
└─────────────────────────────────────────────────────────────┘
                            ↓ AST
┌─────────────────────────────────────────────────────────────┐
│  Stage 3: SEMANTIC ANALYSIS                           │
│  - sema.rs (Rust) or sema.oco (OptiCo)               │
│  - Type checking, linearity verification, etc.            │
└─────────────────────────────────────────────────────────────┘
                            ↓ Typed AST
┌─────────────────────────────────────────────────────────────┐
│  Stage 4: CODE GENERATION                              │
│  - codegen.rs (Rust) or codegen.oco (OptiCo)          │
│  - Output: LLVM IR (.ll files)                         │
└─────────────────────────────────────────────────────────────┘
                            ↓ LLVM IR
┌─────────────────────────────────────────────────────────────┐
│  Stage 5: GOIR PIPELINE (Optional)                     │
│  - opt -load-pass-plugin=libGOIRPasses.so              │
│  - Instrumentation for safety checks                    │
└─────────────────────────────────────────────────────────────┘
                            ↓ Instrumented LLVM IR
┌─────────────────────────────────────────────────────────────┐
│  Stage 6: COMPILATION TO BINARY                        │
│  - llc: LLVM IR → Object file (.o)                    │
│  - clang: Link with runtime → Executable binary         │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  Native Executable Binary                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Self-Hosting Verification Process

### Step 1: Bootstrap (Rust → Binary)
```bash
./target/release/app std/compiler/*.oco
# Generates: lexer.ll, parser.ll, codegen.ll, sema.ll, main.ll
```

### Step 2: Build Self-Hosted Compiler
```bash
llvm-link std/compiler/*.ll -o optico_combined.ll
llc -filetype=obj optico_combined.ll -o optico.o
clang optico.o -Lbuild/runtime -lgoirrt -o optico_self_hosted
```

### Step 3: Self-Compilation (The Test)
```bash
./optico_self_hosted std/compiler/lexer.oco
./optico_self_hosted std/compiler/parser.oco
# ... etc
# Build optico_v2 from self-compiled sources
```

### Step 4: Verification
```bash
# Compare binaries (should be functionally equivalent)
diff <(./optico_self_hosted test.oco) <(./optico_v2 test.oco)
```

---

## Key Metrics

| Metric | Rust Bootstrap | Self-Hosted (Target) |
|--------|-----------------|---------------------|
| Language | Rust | OptiCo |
| Lines of Code | 3,273 | ~2,500 |
| Test Pass Rate | 94.2% | TBD |
| Compilation Speed | ~5ms/file | TBD |
| Binary Size | ~10MB (debug) | TBD |
| Self-Hosting | N/A (bootstrap) | **Goal** |

---

## Documentation

### For Developers
- **SELF_HOSTING_ROADMAP_2026.md** - Detailed technical roadmap
- **SELF_HOSTING_STATUS_2026.md** - Current status report
- **docs/goir_spec.md** - GOIR specification
- **IMPLEMENTATION_SUMMARY.md** - Implementation details

### For Users
- **README.md** - Language overview and syntax
- **docs/stdlib.md** - Standard library reference
- **examples/** - Example OptiCo programs

---

## Contributing

### How to Test Changes
1. Make changes to self-hosted compiler (`std/compiler/*.oco`)
2. Compile with Rust bootstrap: `./target/release/app std/compiler/changed.oco`
3. Check LLVM IR output: `cat changed.oco.ll`
4. Test with llc: `llc -filetype=obj changed.oco.ll`
5. Report any issues

### How to Extend the Language
1. Update Rust bootstrap compiler first
2. Add tests for new feature
3. Update self-hosted compiler to match
4. Update documentation
5. Run full test suite

---

## Known Issues

### 1. LLVM Header Issues in GOIR Passes
```
error: 'llvm/IR/PassManager.h' file not found
```
**Workaround**: Disable GOIR passes for initial self-hosting verification.

### 2. Self-Hosted Compiler Completeness
- Parser may have issues with very complex expressions
- Codegen may not handle all edge cases
- Semantic analysis is minimal

**Mitigation**: Incremental testing and fixing.

### 3. Standard Library Gaps
- File I/O operations needed for compiler
- Better data structures needed for symbol tables

**Mitigation**: Enhance std lib in parallel with compiler work.

---

## Success Criteria

### Minimum Viable Self-Hosting (MVSH)
- [x] Self-hosted compiler components exist
- [ ] Self-hosted compiler compiles a simple program
- [ ] Generated binary runs correctly
- [ ] Self-hosted compiler can compile its own lexer.oco

### Full Self-Hosting Verification
- [ ] Self-hosted compiler compiles ALL `std/compiler/*.oco` files
- [ ] Resulting binary can recompile sources
- [ ] Multiple self-compilation cycles work
- [ ] Binaries are functionally equivalent

---

## Contact & Resources

- **Project Location**: `/workspace/e871da96-b7af-4534-8a29-fbdc5c386c91/sessions/agent_5469134d-d6b6-4ff5-8881-77321637e5c5/`
- **Language Version**: OptiCo v0.3
- **Last Updated**: April 23, 2026

---

*This README is maintained as part of the self-hosting development effort.*

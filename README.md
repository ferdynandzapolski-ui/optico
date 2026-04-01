# OptiCo v0.3: Coalgebraic Optics Compiler

OptiCo is an imperative systems programming language that achieves memory and resource safety by replacing raw pointers and manual lifetime management with First-Class Coalgebraic Optics. Safety is the default, enforced at compile-time by treating all data access as lawful transformations within a comonadic store.

## 1. Theoretical Foundation: The Safety Kernel

The core paradigm of OptiCo is that every memory access is a coalgebra for the costate (store) comonad. For a view type $V$ and a source $S$, the comonad is defined as:

$$W_V(S) = V \times (V \rightarrow S)$$

A lawful optic is a function $\alpha: S \rightarrow W_V(S)$ that satisfies three axioms corresponding to the lens laws:

| Lens Law | Formal Requirement | Operational Meaning |
| :--- | :--- | :--- |
| Get-Put | $put(s, get(s)) = s$ | Reading and writing back the same value is a no-op. |
| Put-Get | $get(put(s, v)) = v$ | A focus retrieves exactly what was last written to it. |
| Put-Put | $put(put(s, v_1), v_2) = put(s, v_2)$ | Final writes overwrite previous intermediate writes. |

OptiCo v0.3 extends these laws with **Traversal Disjointness**:

$$get(o_1)(put(o_2)(v, s)) = get(o_1)(s)$$

This ensures that mutation through one optic never affects the view of a disjoint optic, effectively preventing data races and unexpected aliasing.

## 2. Refined Abstract Syntax (BNF v0.3)

The syntax introduces types for comonadic contexts, coinductive recursion, and the Nakano "later" modality for productivity.

```bnf
τ ::= Int | Float | Bool | Char | Void
 | Struct{l_i : τ_i} | Resource Δ [Σ] // Durability Δ, Initial State Σ
 | co Δ <ContextID> τ       // Comonadic context (Store) with Durability Δ and Context
 | optic τ*                 // Coalgebraic view
 | traversal τ*             // Coalgebraic array view
 | I τ                      // Nakano 'later' modality
 | rec optic τ*             // Coinductive recursive optic
 | atomic optic τ*          // Verified thread-safe optic
 | pointer<τ, ContextID>    // Phantom-lifetime raw pointer

Δ ::= Volatile | Normal | Durable

e ::= x | C | e.l | e₁ | e₂
 | next e                   // Guarded recursion delay
 | prev e                   // Removal of later modality
 | return e                 // Return from function
 | alloc<τ, Δ>() | free(e)  // Comonadic management with Durability
 | *e | e₁ := e₂             // Lawful Get/Put
 | resource r = e           // Must-consume linear resource
 | unsafe { e }             // Guarded by phantom tokens
 | checked (e)              // Runtime disjointness guard
 | spawn { e }              // Multi-threaded execution
```

## 3. Advanced Safety Mechanisms

### 3.1. Productivity via Guarded Recursion
To prevent infinite loops during data retrieval, recursive optics must be productive. v0.3 uses the Nakano later modality ($I$) to ensure that recursive calls are nested under a delay (`next`), shifting the availability of the result to a future "clock tick".

### 3.2. Concurrency via Atomic Optics
When a context is shared across threads via `spawn`, the compiler requires views into it to be `atomic optic` types. The compiler wraps the `put` operation in hardware-assisted synchronization (CAS or mutexes). The SMT backend proves that these concurrent updates still obey the Put-Put and Get-Put laws under interleaving.

**Atomic Promotion**: In multi-threaded `spawn {}` blocks, if the compiler cannot prove that two optics are disjoint across threads, it automatically promotes the views to `atomic optic` types. This wraps the `put` operation in hardware-assisted synchronization (such as a Compare-And-Swap loop or a mutex) to ensure the lens laws are preserved under concurrent access.

### 3.3. Linear Resources and SSFG Analysis
Resources (files, locks) are strictly linear; they must be consumed exactly once. v0.3 uses Source-Sink Flow Graphs (SSFG) to build a reachability map from resource allocation (Source) to consuming optic (Sink). SSFG reduction allows the compiler to verify linearity in linear time. The **SSFG Checkpointing** mechanism persists the flow graph state to disk, allowing for cross-restart linearity verification.

**Reachability Heuristic**: The compiler uses a reachability heuristic on the SSFG to prove the "must-consume" invariant in linear time. By defining allocation as a "Source" and terminal operations (like `close` or a consuming `put`) as a "Sink," the compiler ensures every resource reaches a terminal state before scope exit.

### 3.4. Incremental Computation: Zero-Copy CAS + LMDB
The compiler uses a **Content-Addressable Storage (CAS)** system for IR persistence, backed by the **LMDB** memory-mapped database. This provides:
- **Zero-Copy Reads**: Page-cache mapped IR nodes accessed directly via memory mapping.
- **Red-Green Early Cutoff**: 128-bit **Fingerprinting** of query results halts propagation if re-computed hashes match cached values.
- **Tiered Durability**: `Durable` nodes (StdLib) stay paged, `Normal` nodes follow an **LRU eviction policy**, and `Volatile` nodes reside in hot memory.

### 3.5. Automation of Runtime Checks
The compiler automatically injects runtime checks in cases where the SMT solver returns a result of "unprovable" or "unknown" for a safety constraint.
- **Checked Optic Fallback**: For failure modes such as Traversal Overlap (e.g., dynamic indices `arr[i]` vs `arr[j]`), the compiler attempts to prove $i \neq j$ statically. If the proof fails due to complex runtime logic, the compiler transforms the access into a `checked optic`, which inserts a deterministic $O(1)$ runtime check to ensure disjointness.
- **Context Invalidation**: All comonadic contexts include an `active` flag. The compiler instruments every `get` and `put` with a check for this flag, automatically trapping use-after-free errors at runtime if the context's `counit` has already executed.

## 4. The Verification Pipeline

The compiler uses a Tiered SMT Coordinator to discharge verification conditions (VCs).

| Tier | Optic Complexity | Prover Hardware | Methodology |
| :--- | :--- | :--- | :--- |
| 1 | Syntactic Field Lenses | CPU Frontend | $O(1)$ offset calculation. |
| 2 | Local Compositions | Parallel CPU | Normalized SMT solving. |
| 3 | Traversals / Atomic | GPU (ParaFROST) | Data-parallel formula simplification. |
| 4 | Recursive Optics | GPU Hybrid | Coinductive clock-quantified proofs. |

### Deterministic SMT Normalization
To address proof instability, v0.3 canonicalizes all VCs before solving via de-shadowing, assertion sorting, and fixed seeds derived from the VC hash.

## 5. Compiler Architecture (Implementation)

The compiler is implemented in Rust and consists of the following components:

- **Lexer (`src/lexer.rs`)**: Tokenizes the input source code.
- **Parser (`src/parser.rs`)**: A recursive descent parser that builds an AST.
- **AST (`src/ast.rs`)**: Defines the Abstract Syntax Tree and type system.
- **Semantic Analyzer (`src/sema.rs`)**: Implements type checking, comonadic type verification, productivity analysis (Nakano later modality), and linearity checking.
- **Coalgebraic IR (`src/cir.rs`)**: Lowers the AST to a mid-level representation that treats memory as a comonadic store.
- **Code Generator (`src/codegen.rs`)**: Generates output from the CIR.

## 6. Implementation Roadmap

- **Phase 1: Frontend & Specification (v0.3 Complete)**: EBNF Grammar and Lean 4 formalization.
- **Phase 2: Coalgebraic IR (CIR) & Static Analysis**: CIR lowering and SSFG Solver.
- **Phase 3: The Hardened Proof Backend**: Tiered Proof Coordinator with ParaFROST integration.
- **Phase 4: LLVM Backend & Core Library**: Optimized LLVM emission and `std.optico`.

## 7. Usage

### Building the Compiler
```bash
cargo build
```

### Running Tests
```bash
cargo test
```

### Running the Compiler
```bash
cargo run
```

## 8. Code Examples

### A. Guarded Productivity (Productive List Sum)
```c
// Guarded recursion ensures the function is productive (terminates for each observation)
rec optic int* list_sum(optic ListNode* node) {
    return node->data | next list_sum(node->next);
}
```

### B. Verified Multi-Threading (Shared Atomic Counter)
```c
atomic optic int* shared_counter;

void main() {
    co int* counter = alloc<int>(0);
    shared_counter = counter; // Promoted to atomic view

    spawn {
        // Atomic Put: verified disjointness prevents races
        *shared_counter := *shared_counter + 1;
    }
}
```

### C. Strictly Linear Resource (SSFG Verification)
```c
resource File* f = fopen("config.bin", "rb");
optic char* first = f->_buffer;

// This 'put' operation consumes the linear token 'f'
*first := 65;

// Any further access to f or first triggers a "Linearity Leak/Reuse" error
```

## 9. Standard C Interoperability (Pointer Lifting)

OptiCo v0.3 supports direct interoperability with standard C code while maintaining safety guarantees through **Pointer Lifting**.

### 9.1. Extern C Blocks
Developers can import C functions and variables using `extern "C"` blocks.

```c
extern C {
    void* malloc(int size);
    void free(void* ptr);
}
```

### 9.2. Formal Intent Inference (Non-LLM)
To infer the "intent" of code (such as memory ownership or resource protocols) without relying on probabilistic models like LLMs, OptiCo utilizes deductive symbolic reasoning and structural heuristics.
- **Bi-abduction**: This technique automates the discovery of specifications from bare code by symbolically executing a function to identify **Antiframes** (missing state required for safety) and **Frames** (state that remains unchanged).
- **Combinatorial Invariant Generation**: For loop-heavy code, the compiler employs combinatorial techniques such as recurrence solving and variable elimination to deduce loop invariants without human intervention.

### 9.3. C Heap Context (`C_context`)
Legacy heap management is modeled using the `C_context` comonadic context. The compiler uses SSFG analysis to ensure that every `malloc` is balanced by a `free`, preventing leaks and double-frees in imported C code.

### 9.4. Phantom Lifetime Stabilization
OptiCo prevents stack pointers from escaping their function's activation record. The compiler uses static "points-to" analysis to bind raw pointers to specific `ContextID` tokens. Any attempt to return a pointer to a local variable or store it in a longer-lived context results in a **Phantom Lifetime Violation**.

## 10. Conclusion

OptiCo v0.3 provides a robust, formally verified alternative to traditional systems languages. By embedding safety directly into the coalgebraic structure of data access and leveraging GPU-accelerated formal methods, it achieves the "zero-cost safety" ideal for next-generation systems development.

# Compiler as a Graph: The Cofree Comonad Model (v0.3)

In OptiCo v0.3, the compiler's Intermediate Representation (IR) is not a set of flat data structures but a **Cofree Comonad** modeled as a state-transitioning graph. This architecture allows for first-class query support, incremental updates, and safe disk-backed persistence.

## 1. Mathematical Structure

The IR is represented as the cofree comonad $D(X) = \nu a. X \times F(a)$:
- **Nodes ($X$):** The carrier set containing local attributes (source location, symbol name).
- **Edges ($F$):** The functor defining connections (parent, child, next instruction).
- **Productivity:** Access is guarded by the later modality ($I$) to ensure termination of recursive queries.

## 2. Graph Representation via Protocols

Each node in the IR follows the `NodeSession` protocol to ensure phase-ordered analysis.

```optico
protocol NodeSession {
    state Parsed {
        optic Symbol* resolve;   // Transitions Parsed -> Resolved
    }
    state Resolved {
        optic Type* typecheck;   // Transitions Resolved -> Typed
    }
    state Typed {
        optic IR* lower;         // Transitions Typed -> Lowered
    }
    state Lowered {}
}
```

### 2.1. Storage Architecture: Zero-Copy CAS + LMDB

To support large codebases, the graph is backed by a **Zero-Copy Content-Addressable Storage (CAS)** system using **LMDB** as a memory-mapped backing engine. This allows the compiler to treat the entire disk-backed graph as a single comonadic store.

- **Content-Addressability:** Each node is identified by a stable 128-bit **Fingerprint** (cryptographic hash).
- **Zero-Copy Reads:** Nodes are paged directly from the OS page cache into the address space, allowing optics to perform field-offset arithmetic directly on disk-mapped data without deserialization.
- **Durability Levels:** Nodes are tagged with durability levels to manage memory pressure:
  - **Volatile:** Temporary nodes residing in the hot heap.
  - **Normal:** Standard IR nodes eligible for paging to disk.
  - **Durable:** Standard library and external dependency nodes, pre-loaded in memory-mapped read-only pages.
- **Interoperability Contexts:** The graph supports legacy C nodes via special contexts:
  - **C_context:** A comonadic wrapper for memory-mapped legacy IR produced by standard C frontends.
- **LRU Paging:** "Cold" nodes are paged into memory on-demand when focused by an optic, with an LRU (Least Recently Used) policy evicting nodes when memory limits are reached.

### 2.2. Incremental Stability: The Red-Green Algorithm

Incremental updates are managed via the **Red-Green Algorithm** for "Early Cutoff" stability:
1. **Fingerprinting:** Every query result is paired with a 128-bit fingerprint of its inputs.
2. **Early Cutoff:** If a node's source changes (marking it "Red"), the compiler re-executes the optic. If the new result fingerprint matches the cached one, the compiler marks the node "Green" and halts propagation.

### 2.3. SSFG Persistence

The **Source-Sink Flow Graph (SSFG)** for linear resources is persisted as a "Checkpointed Thread," allowing the compiler to resume linearity verification across restarts without re-analyzing the entire project. In v0.3, this includes tracking of C heap allocations (`malloc`/`free`) to ensure memory safety in mixed-language graphs.

### 2.4. Formal Intent Inference in the Graph

The compiler leverages the graph-based IR to perform formal intent inference without probabilistic models.
- **Bi-abduction on Nodes**: The compiler symbolically executes functions by traversing the IR graph. This allows it to discover specifications for nodes, identifying **Antiframes** (the state required for a function node to be safely executed) and **Frames** (the part of the graph that remains invariant).
- **SSFG Reachability**: Resource linearity is verified by performing a reachability analysis on the SSFG embedded within the IR. The compiler proves the "must-consume" invariant by ensuring that every "Source" node in the graph reaches a terminal "Sink" node along all execution paths.

## 3. Query as Optic Composition

Queries are implemented as compositions of first-class optics. This removes the need for a separate query language.

```optico
// Find the type of a node by resolving its symbol and checking the type.
rec optic Type* get_node_type(optic Node* n) {
    return n | NodeSession.resolve | NodeSession.typecheck;
}
```

## 4. Standard Library Integration

The standard library (`std/`) must provide the base implementations for these nodes and their associated optics.

| Component | Path | Description |
| :--- | :--- | :--- |
| `Node` | `std/ir/node.oco` | Base AST node structure with `PtrVector` children, graph pointers (`parent`, `next`), and source locations. |
| `Symbol` | `std/ir/symbol.oco` | Symbol table and scope management referencing IR nodes. |
| `Type` | `std/ir/type.oco` | Type system representation supporting parameterized types. |
| `PtrVector` | `std/ptr_vector.oco` | Core collection for managing IR node pointers. |

## 5. Implementation Status (Self-Hosting)

The following components have been implemented in the self-hosted standard library to support the bootstrap process:

- **Lexer (`std/compiler/lexer.oco`):** Full keyword support for OptiCo v0.3, including `PerfGrade`, `extern "C"`, and coinductive `rec optic`. Supports character and string literals with escape sequences.
- **Parser (`std/compiler/parser.oco`):** Implements `parser_parse_program` as the entry point. Supports `extern "C"` blocks, nested struct/protocol declarations, and standard expression precedence (additive/multiplicative).
- **IR Core (`std/ir/`):** Implements `NodeSession` protocol transitions (`resolve`, `typecheck`, `lower`). Includes `SymbolTable` management for scope resolution.
- **Standard Library:** Core data structures (`String`, `Vector`, `PtrVector`, `List`, `Map`) and I/O abstractions (`File`, `Console`) are fully defined and compatible with the Rust-based bootstrap compiler.

### Issues Encountered & Resolved

1. **Tag Collision:** Initial lexer implementation had overlapping tags for some operators. Re-aligned tags with the Rust compiler's internal `Token` enum mapping for consistency.
2. **Recursive Descent in OCO:** Due to the linear nature of resource management in OptiCo, some recursive parsing patterns required explicit state passing. Resolved by utilizing comonadic focus on `Parser` state.
3. **Symbol Table Persistence:** Early designs for `SymbolTable` were volatile. Integrated `PtrVector` for durable symbol storage to support incremental stability (Red-Green algorithm).

## 6. Summary

By treating the compiler as a self-querying graph, OptiCo achieves:
1. **Phase Safety:** Queries fail at compile-time if the IR hasn't reached the required state.
2. **Persistence:** Disk-backed nodes are handled safely via comonadic stores.
3. **Efficiency:** Incremental updates propagate only through the affected optics.

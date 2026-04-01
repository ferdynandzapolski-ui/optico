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
}
```

### 2.1. Disk Backing and Persistence

To support large codebases, the graph can be backed by disk storage using comonadic contexts with specific `ContextID`s.

- **Disk Pointer:** `pointer<Node, "Disk">` represents a node stored in a memory-mapped file or a database.
- **Lazy Loading:** The `co Node*` context transparently manages the transfer between disk and memory via optic-triggered `get` operations.

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
| `Node` | `std/ir/node.oco` | Base AST node structure. |
| `Symbol` | `std/ir/symbol.oco` | Symbol table and scope management. |
| `Type` | `std/ir/type.oco` | Type system representation. |

## 5. Summary

By treating the compiler as a self-querying graph, OptiCo achieves:
1. **Phase Safety:** Queries fail at compile-time if the IR hasn't reached the required state.
2. **Persistence:** Disk-backed nodes are handled safely via comonadic stores.
3. **Efficiency:** Incremental updates propagate only through the affected optics.

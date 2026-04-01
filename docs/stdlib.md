# OptiCo Standard Library (v0.3)

Welcome to the documentation for the OptiCo Standard Library. This library provides fundamental data structures, optics, and resource management tools for writing robust, comonadic programs.

## Prelude (`std/prelude.oco`)
The prelude defines basic types and common optics.

### Structs
- `struct Unit {}`: An empty struct representing a unit type.
- `struct Pair { int fst; int snd; }`: A simple container for two integer values.

### Optics
- `optic int* left(optic Pair* p)`: Focuses on the first element of a `Pair`.
- `optic int* right(optic Pair* p)`: Focuses on the second element of a `Pair`.

## I/O and System Resources (`std/io.oco`)
The I/O module handles interaction with the system.

### Resources
- `resource Console console`: A linear resource representing the system console. It is pre-allocated with an ID of 1.

### Structs
- `struct File { int fd; }`: Represents a file handle.

### Functions
- `void print_int(int val)`: Outputs an integer to the console.
- `void print_char(char val)`: Outputs a character to the console.

## Compiler IR Components (`std/ir/`)
To support the graph-based IR model, the library provides standard structures and protocols for compiler phases.

### Protocols
- `protocol NodeSession`: Defines the sequence `Parsed -> Resolved -> Typed -> Lowered` for IR nodes.
- `state Parsed { optic Symbol* resolve; }`: Focuses on the resolver to reach the `Resolved` state.
- `state Resolved { optic Type* typecheck; }`: Focuses on type analysis to reach the `Typed` state.

### Structs
- `struct Node { int id; SourceLoc loc; }`: The base carrier for AST information.
- `struct Symbol { string name; int scope_id; }`: Represents a resolved symbol.
- `struct Type { string kind; int size; }`: Represents a verified type.

## Interoperability Support (`std/interop.oco`)
Supports legacy C integration through Pointer Lifting and `C_context`.

### Named Contexts
- `C_context`: A special comonadic context for standard `malloc`/`free` operations.

### Lifting Annotations
- `traversal`: Used to annotate C pointers intended for array-style access.

## Usage
Include the standard library files at the beginning of your project or as needed.

```optico
// Example
struct Point { int x; int y; }

void main() {
    co Point* p = alloc<Point>(10, 20);
    int val = p->x;
}
```

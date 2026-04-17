# OptiCo Standard Library (v0.3)

Welcome to the documentation for the OptiCo Standard Library. This library provides fundamental data structures, optics, and resource management tools for writing robust, comonadic programs.

## Prelude (`std/prelude.oco`)
The prelude defines basic types and common optics.

### Structs
- `struct Unit {}`: An empty struct representing a unit type.
- `struct Pair { int fst; int snd; }`: A simple container for two integer values.
- `struct Fingerprint { int hi; int lo; }`: 128-bit Content-Addressable identifier.

### Optics
- `optic int* left(optic Pair* p)`: Focuses on the first element of a `Pair`.
- `optic int* right(optic Pair* p)`: Focuses on the second element of a `Pair`.

## Content-Addressable Storage (`std/cas.oco`)
Provides the base for the Zero-Copy CAS system.

### Functions
- `Fingerprint cas_fingerprint(String s)`: Generates a stable fingerprint for a string.
- `void cas_store(CAS c, Fingerprint fp, pointer<Unit, Heap> data)`: Stores data associated with a fingerprint.
- `pointer<Unit, Heap> cas_fetch(CAS c, Fingerprint fp)`: Retrieves data by its fingerprint.

## I/O and System Resources (`std/io.oco`)
The I/O module handles interaction with the system.

### Resources
- `resource Console console`: A linear resource representing the system console. It is pre-allocated with an ID of 1.

### Structs
- `struct File { int fd; }`: Represents a file handle.

### Functions
- `void print_int(int val)`: Outputs an integer to the console.
- `void print_char(char val)`: Outputs a character to the console.
- `String string_concat(String s1, String s2)`: Concatenates two strings.

## Collections
### PtrVector (`std/ptr_vector.oco`)
- `PtrVector ptr_vector_new(int cap)`: Creates a new pointer vector with initial capacity.
- `void ptr_vector_push(PtrVector v, pointer<Unit, Heap> val)`: Pushes a pointer onto the vector, resizing if necessary.
- `void ptr_vector_remove(PtrVector v, int index)`: Removes an element and shifts subsequent elements.

### Vector (`std/vector.oco`)
- `Vector vector_new(int cap)`: Creates a new integer vector.
- `void vector_push(Vector v, int val)`: Pushes an integer onto the vector, resizing if necessary.
- `bool vector_contains(Vector v, int val)`: Checks if a value exists in the vector.

## Compiler IR Components (`std/ir/`)
To support the graph-based IR model, the library provides standard structures and protocols for compiler phases.

### Protocols
- `protocol NodeSession`: Defines the sequence `Parsed -> Resolved -> Typed -> Lowered` for IR nodes.
- `state Parsed { optic Symbol* resolve; }`: Focuses on the resolver to reach the `Resolved` state.
- `state Resolved { optic TypeIR* typecheck; }`: Focuses on type analysis to reach the `Typed` state.

### Structs
- `struct Node { int id; int kind; String lexeme; PtrVector children; ... Fingerprint fingerprint; int durability; }`: The base carrier for AST information.
- `struct Symbol { String name; ... Fingerprint fingerprint; }`: Represents a resolved symbol.
- `struct TypeIR { String kind; ... Fingerprint fingerprint; }`: Represents a verified type.

## Interoperability Support (`std/interop.oco`)
Supports legacy C integration through Pointer Lifting and `C_context`.

### Named Contexts
- `C_context`: A special comonadic context for standard `malloc`/`free` operations. This context includes an `active` flag used for runtime safety instrumentation to detect and trap use-after-free errors.

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

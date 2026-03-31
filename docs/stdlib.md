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

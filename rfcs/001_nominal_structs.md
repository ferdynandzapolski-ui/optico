# RFC 001: Nominal Struct Declarations

## Status
Proposed

## Context
Currently, OptiCo v0.3 uses structural representations or placeholders for structs. To build a robust standard library and allow users to define complex data structures, we need nominal struct declarations.

## Proposal
Introduce a new top-level declaration for structs:

```optico
struct Point {
    int x;
    int y;
}
```

### Syntax
The `struct` keyword followed by an identifier and a brace-enclosed list of field declarations. Each field consists of a type and an identifier, followed by a semicolon.

### Semantic Analysis
1. **Registration**: When the semantic analyzer encounters a `struct` declaration, it will store the struct name and its field definitions in a global symbol table.
2. **Nominal Typing**: A new `Type::Named(String)` variant will represent a reference to a declared struct.
3. **Field Access**: When checking `Expr::Access(expr, field_name)`, the analyzer will:
   - Determine the type of `expr`.
   - If it is a `Type::Named(name)`, it will lookup the fields of struct `name`.
   - If `field_name` exists, the expression's type will be the type of that field.

## Benefits
- Improved type safety.
- Clearer intention for complex data models.
- Foundational for the standard library (e.g., `ListNode`, `FileHandle`).

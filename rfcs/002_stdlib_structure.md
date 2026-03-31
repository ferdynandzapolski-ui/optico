# RFC 002: Standard Library Structure and Core Resources

## Status
Proposed

## Context
OptiCo needs a basic standard library to support common operations and provide abstractions for hardware and system resources.

## Proposal

### Directory Structure
A new top-level directory `std/` will contain standard library files.

### File Extension
The official file extension for OptiCo source files will be `.oco`.

### Core Modules
1. **`std/prelude.oco`**: Automatically included or a common starting point for all programs.
   - Includes basic optics like `id` and `fst`, `snd`.
2. **`std/io.oco`**: Definitions for Input/Output.
   - `Console`: A linear resource for writing and reading from the terminal.
   - `File`: A linear resource for file system operations.

### Example Definitions
#### `std/prelude.oco`
```optico
// Placeholder for common optics
```

#### `std/io.oco`
```optico
struct File {
    int handle;
}

resource File open(char* path) {
    // intrinsic or basic logic
}
```

## Benefits
- Promotes code reuse and organization.
- Clearly defines how to interact with system resources.
- Establishes conventions for project structure.

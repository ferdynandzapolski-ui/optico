# GOIR Build System

## Prerequisites
- CMake (>= 3.24)
- Ninja
- LLVM/Clang (>= 18)
- Python 3

## Build Instructions
1. Create a build directory: `mkdir build && cd build`
2. Configure with CMake: `cmake -G Ninja ..`
3. Build the project: `ninja`

## Running Tests
- Use `ninja check-goir` to run lit tests.

#include <stdio.h>
#include <string.h>

// External function from OptiCo
int compile(char* source);

// Define print_int for the LLVM IR
void print_int(int x) {
    printf("Debug: %d\n", x);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s \"expression\"\n", argv[0]);
        printf("Example: %s \"1+2\"\n", argv[0]);
        return 1;
    }

    char* expr = argv[1];
    printf("Compiling expression: %s\n", expr);
    int result = compile(expr);
    printf("Result: %d\n", result);
    return 0;
}

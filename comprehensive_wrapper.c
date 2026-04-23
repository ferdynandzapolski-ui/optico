#include <stdio.h>
#include <string.h>

// External functions from OptiCo
int compile(char* source);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s \"expression\"\n", argv[0]);
        printf("Example: %s \"1+2*3\"\n", argv[0]);
        return 1;
    }

    char* expr = argv[1];
    printf("Compiling expression: %s\n", expr);
    int result = compile(expr);
    printf("Result: %d\n", result);
    return 0;
}

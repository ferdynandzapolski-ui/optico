#include <stdio.h>
#include <string.h>

// External print function
void print_int(int x) {
    printf("%d\n", x);
}

// External functions from OptiCo
void test_variables();
int compile(char* source);

int main(int argc, char* argv[]) {
    if (argc == 1) {
        // Test variables
        printf("Testing variables:\n");
        test_variables();
        printf("\n");
    } else if (argc == 2) {
        // Compile expression
        char* expr = argv[1];
        printf("Compiling expression: %s\n", expr);
        int result = compile(expr);
        printf("Result: %d\n", result);
    } else {
        printf("Usage: %s [expression]\n", argv[0]);
        printf("  No args: test variables\n");
        printf("  With arg: compile expression\n");
        return 1;
    }
    return 0;
}

#include <stdio.h>
#include <stdlib.h>

extern int compile(char* source);

void print_int(int val) {
    printf("%d\n", val);
}

int main() {
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 1;
    }
    compile(buffer);
    return 0;
}
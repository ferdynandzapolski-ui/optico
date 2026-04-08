#include <stdio.h>

void __go_check_load(void* ptr) {
    printf("GOIR check load: %p\n", ptr);
}

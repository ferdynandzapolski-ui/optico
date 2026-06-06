#include <stdio.h>
#include <stdlib.h>
#include "goirrt.h"

int main() {
    go_grade_t g;
    int *arr = (int*)__go_malloc(5 * sizeof(int), &g);
    uint64_t addr = (uint64_t)arr;

    printf("Exposing pointer %p\n", arr);
    __go_prov_expose(arr, g, 1); // PNVI-ae expose

    printf("Resolving integer 0x%lx (Policy 1: PNVI-ae)...\n", addr);
    ptr_grade_res_t res = __go_inttoptr_resolve(addr, 1, 0);
    if (res.g.end == -1ULL) {
        printf("FAILED to resolve (unexpected for exposed)\n");
    } else {
        printf("SUCCESSFULLY resolved exposed pointer\n");
    }

    uint64_t unexposed_addr = addr + 0x1000; // Not a live object
    printf("Resolving unexposed integer 0x%lx...\n", unexposed_addr);
    res = __go_inttoptr_resolve(unexposed_addr, 1, 0);
    if (res.g.end == -1ULL) {
        printf("SUCCESSFULLY blocked unexposed/invalid resolution\n");
    } else {
        printf("FAILED: resolved invalid address\n");
    }

    __go_free(arr, g);
    return 0;
}

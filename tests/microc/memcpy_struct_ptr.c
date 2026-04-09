#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct Node {
    int val;
    struct Node *next;
};

int main() {
    struct Node *src = (struct Node*)malloc(sizeof(struct Node));
    src->val = 42;
    src->next = NULL;

    struct Node *dst = (struct Node*)malloc(sizeof(struct Node));

    printf("Memcpy of struct with pointer field...\n");
    memcpy(dst, src, sizeof(struct Node));

    if (dst->val == 42 && dst->next == NULL) {
        printf("Success\n");
    }

    free(src);
    free(dst);
    return 0;
}

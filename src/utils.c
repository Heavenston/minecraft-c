#include "safe_cast.h"

#include <stdio.h>
#include <stdlib.h>

void panic(const char *str) {
    fprintf(stderr, "PANIC: %s\n", str);
    abort();
}

static void swap(size_t *a, size_t *b) {
    size_t temp = *a;
    *a = *b;
    *b = temp;
}

void shuffle(size_t n, size_t arr[n]) {
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = safe_to_size_t(rand()) % (i + 1);
        swap(&arr[i], &arr[j]);
    }
}

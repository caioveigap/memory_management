#include "multipool_alloc.h"
#include <stdio.h>
#include <string.h>

int *foo(Pool *pool) {
    int *result = pool_alloc(pool);
    return result;
}

int main() {
    Allocator *malloc = allocator_create();
    Pool *p = pool_create(malloc, (64 * sizeof(int)), DEFAULT_ALIGNMENT);

    int *array = foo(p);

    for (size_t i = 0; i < 64; i++) {
        array[i] = i * 10;
    }

    for (size_t i = 0; i < 64; i++) {
        printf("%d ", array[i]);
    }
}
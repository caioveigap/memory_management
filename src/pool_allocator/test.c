#include "pool.h"
#include <stdio.h>
#include <string.h>


int main() {
    Allocator *alloc = allocator_create();
    Pool *p = pool_create(alloc, 80, 32, DEFAULT_ALIGNMENT);

    int *array_list[20];

    for (size_t i = 0; i < 20; i++) {
        array_list[i] = pool_alloc(p);
        memset(array_list[i], 0xFF, 80);       
    }

    
    int *array = array_list[10];
    printf("array ptr: %p\n", array);

    pool_free((void*)array);

    int *new_array = pool_alloc(p);
    printf("new array ptr: %p\n", new_array);

    pool_destroy(p);
}
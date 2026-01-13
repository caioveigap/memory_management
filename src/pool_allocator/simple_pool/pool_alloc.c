#include "pool_alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

void pool_init(Pool *pool, size_t chunck_initial_count) {
    pool->chunck_count = chunck_initial_count;

    pool->chunck_array = pool->free_chunck = malloc(chunck_initial_count * sizeof(Chunck));
    if (pool->chunck_array == NULL) {
        free(pool);
    }

    for (size_t i = 0; i < chunck_initial_count - 1; i++) {
        pool->chunck_array[i].next = &(pool->chunck_array[i+1]);
    }
    pool->chunck_array[chunck_initial_count - 1].next = NULL;
}

void *pool_alloc(Pool *pool) {
    if (pool == NULL || pool->free_chunck == NULL) {
        return NULL;
    }

    Chunck *result = pool->free_chunck;
    pool->free_chunck = pool->free_chunck->next;
    return result;
}

void pool_free(Pool *pool, void *ptr) {
    if (pool == NULL || ptr == NULL)
        return;

    Chunck *freed = ptr;
    freed->next = pool->free_chunck;
    pool->free_chunck = freed;
}

void pool_destroy(Pool *pool) {
    if (pool == NULL)
        return;

    free(pool->chunck_array);
    free(pool);
}
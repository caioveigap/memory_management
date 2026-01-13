#pragma once
#include <stddef.h>

#define CHUNCK_SIZE 64


typedef union Chunck Chunck;
union Chunck {
    Chunck *next;
    unsigned char buffer[CHUNCK_SIZE];
};

typedef struct Pool Pool;
struct Pool {
    size_t chunck_count;

    Chunck *free_chunck;
    Chunck *chunck_array;
};

void pool_init(Pool *pool, size_t chunck_initial_count);
void *pool_alloc(Pool *pool);
void pool_free(Pool *pool, void *ptr);
void pool_destroy(Pool *pool);
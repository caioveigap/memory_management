#pragma once
#include <stddef.h>
#include <unistd.h>

#define DEFAULT_ALIGNMENT (2 * sizeof(void*))
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define POOL_ALLOCATION_LIMIT 32
#define MAX_GENERIC_POOLS 7
#define MAX_CUSTOM_POOLS 64
#define INITIAL_HEAP_SIZE 1024 * 1024 * 16

// typedef uint8_t u8;

typedef struct Allocator Allocator;
typedef struct Pool_Chunck Pool_Chunck;
typedef struct Pool_Block Pool_Block;
typedef struct Pool Pool;

/* 
Generic Pools: 8 bytes | 16 bytes | 32 bytes | 64 bytes | 128 bytes | 256 bytes | 512 bytes

Maior que 512 bytes, utilizaremos outra estratégia de alocação
*/


struct Pool_Block {
    Pool_Block *next;
};

struct Pool_Chunck {
    struct Pool_Chunck *next;
};

struct Pool {
    size_t pool_size;
    size_t block_size;
    size_t alignment;

    Pool_Block *free_list;
    void *pool_start;
};

struct Allocator {
    void *heap_start;
    size_t heap_capacity;
    size_t heap_offset;
    
    size_t num_pools;
    Pool generic_pools[MAX_GENERIC_POOLS]; 
    Pool custom_pools[MAX_CUSTOM_POOLS];
};

Allocator *allocator_create();
Pool *pool_create(Allocator *allocator, size_t block_size, size_t block_count, size_t alignment);
void *pool_alloc(Pool *p);
void pool_free(Pool *p);
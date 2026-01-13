#pragma once
#include <stddef.h>
#include <unistd.h>

#define DEFAULT_ALIGNMENT (2 * sizeof(void*))
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define POOL_ALLOCATION_LIMIT 32
#define MAX_GENERIC_POOLS 8


typedef struct Allocator Allocator;
typedef struct Page_Allocator Page_Allocator;
typedef struct Pool_Chunck Pool_Chunck;
typedef struct Pool_Block Pool_Block;
typedef struct Pool Pool;

struct Allocator {
    size_t num_pages;
    size_t num_pools;
    Page_Allocator *pages;
    Pool *pools;

    void *heap_start;
    void *heap_top;
    void *heap_end;
};

struct Page_Allocator {
    size_t offset;
    size_t capacity;
    Page_Allocator *next;
};

struct Pool_Block {
    Pool_Block *next;
};

struct Pool_Chunck {
    struct Pool_Chunck *next;
};

struct Pool {
    size_t block_size;
    size_t blocks_per_chunck;
    size_t alignment;

    // Pool_Block *block_array;
    Pool_Block *free_list;
    Pool_Chunck *chuncks;
    char padding[8];
};

Allocator *allocator_create();
Pool *pool_create(Allocator *alloc, size_t block_size, size_t alignment);
void *pool_alloc(Pool *pool);
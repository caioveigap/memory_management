#pragma once
#include <stddef.h>
#include <unistd.h>
#include <stdint.h>

#define DEFAULT_ALIGNMENT (2 * sizeof(void*))
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define POOL_ALLOCATION_LIMIT 32
#define MAX_GENERIC_POOLS 7
#define MAX_CUSTOM_POOLS 32
#define INITIAL_HEAP_SIZE 1024 * 1024 * 32
#define END_BLOCK_FLAG 0x00


typedef uint8_t u8;
typedef uint16_t u16;

typedef struct Allocator Allocator;
typedef struct Chunck Chunck;
typedef struct Block Block;
typedef struct Pool Pool;
typedef struct Page_Header Page_Header;
/* 
Generic Pools: 8 bytes | 16 bytes | 32 bytes | 64 bytes | 128 bytes | 256 bytes | 512 bytes

Maior que 512 bytes, utilizaremos outra estratégia de alocação
*/

struct Page_Header {
    Page_Header *next;
    u16 offset;
    u8 is_free;
};

struct Block {
    Block *next;
};

struct Chunck {
    Chunck *next;
    Pool *parent_pool;
};

struct Pool {
    size_t pool_size;
    size_t block_size;
    size_t alignment;
    
    Chunck *root_chunck;
    Chunck *curr_chunck;

    Block *free_list;

    Allocator *parent_allocator;
};

struct Allocator {
    void *heap_start;
    size_t heap_capacity;
    size_t heap_offset;

    size_t num_pools;
    
    Page_Header *free_pages;

    Pool generic_pools[MAX_GENERIC_POOLS]; 
    Pool custom_pools[MAX_CUSTOM_POOLS];
};

Allocator *allocator_create();
Pool *pool_create(Allocator *allocator, size_t block_size, size_t block_count, size_t alignment);
void *pool_alloc(Pool *p);
void pool_destroy(Pool *pool);
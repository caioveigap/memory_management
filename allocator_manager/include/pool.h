#pragma once
#include <stddef.h>
#include <unistd.h>
#include <stdint.h>

#include "../include/backend_manager.h"

#define POOL_ALLOCATION_LIMIT 32
#define MAX_GENERIC_POOLS 7
#define MAX_CUSTOM_POOLS 32
#define MAX_HEAP_SIZE 1024 * 1024 * 64
#define MAX_POOL_BLOCK_SIZE 512
#define CHUNK_USAGE_TRESHOLD 0.75


typedef uint8_t u8;
typedef uint16_t u16;

typedef struct Allocator Allocator;
typedef struct Pool Pool;

typedef struct Pool_Block {
    struct Pool_Block *next;
} Pool_Block;

typedef struct Pool_Chunk {
    struct Pool_Chunk *next;
    struct Pool_Chunk *prev;

    void *data_start;
    Pool_Block *free_list;

    size_t block_size;
    size_t capacity; // Quantidade total de blocos
    size_t used_count;

    struct Pool *parent_pool;
} Pool_Chunk;

typedef struct Pool {
    Pool_Chunk *head_chunk;
    Pool_Chunk *active_chunk;

    size_t capacity; // Número total de blocos
    size_t block_size;

    u16 alignment;
    u16 chunk_order;

    Allocator *parent_allocator;
} Pool;

typedef struct Allocator {
    size_t total_memory;
    struct Pool generic_pools[MAX_GENERIC_POOLS]; 
    struct Pool custom_pools[MAX_CUSTOM_POOLS];
} Allocator;


Pool *pool_create(size_t block_size);
void *pool_alloc(Pool *p);
void *palloc(size_t size);
void pool_free(void *ptr);
void pool_destroy(Pool *pool);
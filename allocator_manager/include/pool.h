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
#define TARGET_BLOCK_COUNT 128


typedef struct Pool Pool;


Pool *pool_create(size_t block_size);
void *pool_alloc(Pool *p);
void *palloc(size_t size);
void pool_free(void *ptr);
void pool_destroy(Pool *pool);
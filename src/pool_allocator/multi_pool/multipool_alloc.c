#include "multipool_alloc.h"
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

typedef uint8_t u8;

/*
TODO: Criar função pool_grow()
TODO: Criar alocação em pools genéricas
 */

void print_addr(void *ptr, char *label) {
    printf("%s Address: %ld\n", label, (uintptr_t)ptr);
}

bool is_power_of_two(size_t x) {
    return (x & (x-1)) == 0;
}

void *get_ptr_from_offset(void *start, size_t offset) {
    return (void*)((uintptr_t)start + offset);
}

size_t align_size(size_t size, size_t alignment) {
    size_t aligned_size = sizeof(void*); // Mínimo é o ponteiro next
    if (size > aligned_size) aligned_size = size;

    size_t remainder = aligned_size % alignment;
    if (remainder != 0) {
        aligned_size += alignment - remainder;
    }
    return aligned_size; // CORRIGIDO
}
void *align_ptr(void *ptr, size_t alignment) {
    uintptr_t addr = (uintptr_t)ptr;
    size_t remainder = addr % alignment;
    if (remainder == 0) return ptr;
    return (void*)(addr + (alignment - remainder));
}

/* 
TODO: Fix pointer arithmetic and pool allocation

Creates allocator for program use

TODO: Tente entender melhor como criar um alocador geral, e separar os conceitos de Arena, Pools, LargeObjs, e como organizar eles

*/
Allocator *allocator_create() {
    size_t heap_size = INITIAL_HEAP_SIZE; // 16 MiB
    void *heap_base = mmap(NULL, heap_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    size_t initial_alloc_size = align_size(sizeof(Allocator), DEFAULT_ALIGNMENT); 
    mprotect(heap_base, initial_alloc_size, PROT_READ | PROT_WRITE);
    
    Allocator *allocator = (Allocator*)heap_base;
    allocator->heap_start = heap_base;
    allocator->heap_capacity = heap_size;
    allocator->heap_offset = sizeof(Allocator);

    // Init generic pools
    size_t pool_block_size = 8;
    size_t pool_alignment = 8;

    for (size_t i = 0; i < MAX_GENERIC_POOLS; i++) {
        Pool current_pool = allocator->generic_pools[i];
        if (i != 0) pool_alignment = DEFAULT_ALIGNMENT;
        current_pool.alignment = pool_alignment;
        current_pool.block_size = pool_block_size;
        current_pool.pool_size = 0;
        current_pool.free_list = NULL;
        current_pool.pool_start = NULL;
        pool_block_size *= 2;
    }

    for (size_t i = 0; i < MAX_CUSTOM_POOLS; i++) {
        Pool current_pool = allocator->custom_pools[i];
        current_pool.alignment = DEFAULT_ALIGNMENT;
        current_pool.block_size = 0;
        current_pool.pool_size = 0;
        current_pool.free_list = NULL;
        current_pool.pool_start = NULL;
    }

    return allocator;
}

void *get_memory(Allocator *allocator, size_t size) {
    void *curr_addr = allocator->heap_start + allocator->heap_offset;
    
    size_t alloc_size = align_size(size, DEFAULT_ALIGNMENT);
    mprotect(curr_addr, alloc_size, PROT_READ | PROT_WRITE);

    allocator->heap_offset += alloc_size;

    return curr_addr;
}

Pool *pool_create(Allocator *allocator, size_t block_size, size_t block_count, size_t alignment) {
    // Search for free custom pool
    size_t i = 0;
    Pool *cp = &allocator->custom_pools[i];

    while (cp->pool_size == 0) {
        cp = &allocator->custom_pools[++i];
    }

    size_t bs = align_size(block_size, alignment);
    size_t alloc_size = bs * block_count;
    
    cp->pool_start = get_memory(allocator, alloc_size);
    
    cp->pool_size = alloc_size;
    cp->alignment = alignment;
    cp->block_size = bs;
    
    // Dividing memory into blocks
    void *memory_start = cp->pool_start; 
    Pool_Block *curr_block = (Pool_Block*)memory_start;
    cp->free_list = curr_block;

    for (size_t i = 0; i < block_count - 1; i++) {
        curr_block->next = (Pool_Block*)((u8*)curr_block + cp->block_size);
        curr_block = curr_block->next;    
    }
    curr_block->next = NULL;

    return cp;
}

Pool *pool_alloc(Pool *p) {
       Pool_Block *result = p->free_list;
    p->free_list = result->next;
    
    return result;
}

void pool_free(Pool *p) {
    p->pool_size = 0;
}


void *palloc() {
    /* 
    Automatic allocation for generic pools
    ...
    */
}

void *pool_alloc(Pool *pool) {
    Pool_Block *block = pool->free_list;
    pool->free_list = block->next;

    return (void*)block;
}
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>

#include "../include/pool.h"
#include "../include/utilities.h"



/* 
==============================
    ESTRUTURAS PRINCIPAIS
==============================
*/

typedef struct Allocator Allocator;

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


// DECLARAÇÕES
Pool_Chunk *get_chunk(size_t size);
Pool_Block *slice_pool_blocks(Pool_Chunk *chunk);
void pool_get_memory(Pool *pool);
u16 calculate_optimal_chunk_order(size_t block_size);
static inline int get_pool_index_from_size(size_t size);
void ensure_allocator_initialized();

static Allocator *global_allocator = NULL;

/* 
==============================
    FUNÇÕES PRINCIPAIS
==============================
*/

Allocator *allocator_create() {
    void *root_base = backend_alloc(PAGE_SIZE, OWNER_POOL);
    if (!root_base) return NULL;
    Allocator *allocator = (Allocator*)root_base;
    allocator->total_memory = 0;

    // Init generic pools
    size_t pool_block_size = 8;
    size_t pool_alignment = 8;

    for (size_t i = 0; i < MAX_GENERIC_POOLS; i++) {
        Pool *current_pool = &allocator->generic_pools[i];
        if (i != 0) pool_alignment = DEFAULT_ALIGNMENT;
        current_pool->alignment = pool_alignment;
        current_pool->block_size = pool_block_size;
        current_pool->capacity = 0;
        current_pool->parent_allocator = allocator;
        current_pool->head_chunk = NULL;
        current_pool->active_chunk = NULL;
        current_pool->chunk_order = calculate_optimal_chunk_order(pool_block_size);
        pool_block_size *= 2;
    }

    for (size_t i = 0; i < MAX_CUSTOM_POOLS; i++) {
        Pool *current_pool = &allocator->custom_pools[i];
        current_pool->alignment = DEFAULT_ALIGNMENT;
        current_pool->block_size = 0;
        current_pool->capacity = 0;
        current_pool->parent_allocator = allocator;
        current_pool->head_chunk = NULL;
        current_pool->active_chunk = NULL;
    }

    return allocator;
}

Pool *pool_create(size_t block_size) {
    ensure_allocator_initialized();

    if (block_size > MAX_POOL_BLOCK_SIZE) {
        fprintf(stderr, "Error [%s]: Requested size can't be larger than 512 bytes.\n", __func__);
        return NULL;
    }
    
    // Search for free custom pool
    size_t alignment = (block_size < DEFAULT_ALIGNMENT) ? 8 : DEFAULT_ALIGNMENT;
    size_t align_block_size = align_size(block_size, alignment);
    size_t chunk_order = calculate_optimal_chunk_order(align_block_size);

    for (size_t i = 0; i < MAX_CUSTOM_POOLS; i++) {
        if (global_allocator->custom_pools[i].capacity == 0) {
            // Achou slot livre
            Pool *pool = &global_allocator->custom_pools[i];
            
            pool->parent_allocator = global_allocator; 
            pool->chunk_order = chunk_order;
            pool->alignment = alignment;
            pool->block_size = align_block_size;
            pool->parent_allocator = global_allocator;
            pool->capacity = 0;
            pool_get_memory(pool);
            return pool;
        }
    }

    fprintf(stderr, "Error [%s]: Out of custom pools!\n", __func__);
    return NULL;
}

void *pool_alloc(Pool *pool) {
    if (pool == NULL) {
        fprintf(stderr, "Error: Tried to allocate on invalid pool\n");
        return NULL;
    }
    if (pool->active_chunk == NULL || pool->active_chunk->free_list == NULL) {
        pool_get_memory(pool);
    }

    Pool_Chunk *selected_chunck = pool->active_chunk;
    Pool_Block *block = selected_chunck->free_list;
    selected_chunck->free_list = block->next;
    selected_chunck->used_count++;

    return (void*)block;
}


void pool_free(void *ptr) {
    Page_Descriptor *chunk_descriptor = get_descriptor(ptr);
    if (chunk_descriptor == NULL) return;

    Pool_Chunk *owner_chunk = (Pool_Chunk*)chunk_descriptor->zone_header;

    size_t chunk_byte_size = (1 << chunk_descriptor->order) * PAGE_SIZE;
    assert(((ptr > (void*)owner_chunk) && (ptr < (void*)((u8*)owner_chunk + chunk_byte_size)))); // Sanity Check

    Pool_Block *freed_block = (Pool_Block*)ptr; 
    freed_block->next = owner_chunk->free_list;
    owner_chunk->free_list = freed_block;
    owner_chunk->used_count--;

    // Verificar se chunk está vazio e devolve ao backend
    if (owner_chunk->used_count == 0) {
        Pool_Chunk *prev = owner_chunk->prev;
        Pool_Chunk *next = owner_chunk->next;
        if (prev == NULL) return;


        f32 prev_usage_percentage =  (f32)prev->used_count / (f32)prev->capacity;
        if (prev_usage_percentage >= CHUNK_USAGE_TRESHOLD) return;

        prev->next = next;

        if (next != NULL) {
            next->prev = prev;
        }

        Pool *parent = owner_chunk->parent_pool;
        if (parent->active_chunk == owner_chunk) {
            parent->active_chunk = prev;
        }

        backend_free(owner_chunk);
    }
}

void *palloc(size_t size) {
    ensure_allocator_initialized();

    if (size == 0) return NULL;
    if (size > MAX_POOL_BLOCK_SIZE) {
        fprintf(stderr, "Error: Pool cannot allocate more than 512 bytes\n");
        return NULL;
    }

    int index = get_pool_index_from_size(size);

    Pool *target_pool = &global_allocator->generic_pools[index];

    return pool_alloc(target_pool);
}

void pool_destroy(Pool *pool) {
    if (pool == NULL) return;

    Pool_Chunk *curr = pool->head_chunk;
    
    while (curr != NULL) {
        Pool_Chunk *next_chunk = curr->next;
        backend_free(curr);
        curr = next_chunk;
    }

    pool->head_chunk = NULL;
    pool->active_chunk = NULL;
    
    pool->capacity = 0;       // Marca o slot como "livre" no array do Allocator
    pool->block_size = 0;
    pool->chunk_order = 0;
    pool->alignment = 0;
}


/* 
==============================
FUNÇÕES AUXILIARES
==============================
*/


Pool_Chunk *get_chunk(size_t size) {
    return (Pool_Chunk*)backend_alloc(size, OWNER_POOL);
}

Pool_Block *slice_pool_blocks(Pool_Chunk *chunk) {
    assert(chunk != NULL && "Invalid chunk pointer passed as argument");

    u8 *curr_ptr = chunk->data_start;
    Pool_Block *fl_head = (Pool_Block*)curr_ptr;
    
    for (size_t i = 0; i < chunk->capacity - 1; i++) {
        // fl_head[i].next = &fl_head[i+1];
        Pool_Block *curr_block = (Pool_Block*)curr_ptr;
        Pool_Block *next_block = (Pool_Block*)(curr_ptr + chunk->block_size);

        curr_block->next = next_block;

        curr_ptr += chunk->block_size;
    }
    Pool_Block *last_block = (Pool_Block*)curr_ptr;
    last_block->next = NULL;

    return fl_head;
}

void pool_get_memory(Pool *pool) {
    Pool_Chunk *new_chunk = get_chunk(REQUEST_SIZE_FROM_ORDER(pool->chunk_order));
    
    if (new_chunk == NULL) {
        fprintf(stderr, "Error: Could not allocate chunk\n");
        return;
    }

    Page_Descriptor *chunk_desc = get_descriptor(new_chunk);

    size_t padding = align_size(sizeof(Pool_Chunk), DEFAULT_ALIGNMENT);
    size_t chunk_capacity = (((1 << chunk_desc->order) * PAGE_SIZE) - padding) / pool->block_size;
    pool->capacity += chunk_capacity;
    
    new_chunk->block_size = pool->block_size;
    new_chunk->capacity = chunk_capacity;
    new_chunk->used_count = 0;
    new_chunk->data_start = (void*)((u8*)new_chunk + padding);
    new_chunk->parent_pool = pool;

    new_chunk->free_list = slice_pool_blocks(new_chunk);

    // Inserir novo chunk na pool
    Pool_Chunk *active = pool->active_chunk;

    if (active == NULL) {
        new_chunk->next = NULL;
        new_chunk->prev = NULL;
        pool->head_chunk = new_chunk;
        pool->active_chunk = new_chunk;
        return;
    }

    new_chunk->next = active->next;
    new_chunk->prev = active;

    if (active->next) {
        active->next->prev = new_chunk;
    } 
    active->next = new_chunk;

    pool->active_chunk = new_chunk;
}

u16 calculate_optimal_chunk_order(size_t block_size) {
    size_t header_size = sizeof(Pool_Chunk);
    size_t target_count = 128; 
    size_t ideal_size = header_size + (block_size * target_count);
    
    if (ideal_size < 16384) ideal_size = 16384; // Tamanho mínimo da Pool é de 16KB
    size_t order = get_order(ideal_size);

    if (order > MAX_BIN_ORDER) {
        order = MAX_BIN_ORDER;
    }

    return (u16)order;
}

static inline int get_pool_index_from_size(size_t size) {
    if (size > 512) return -1;

    return fast_log2(round_up_pow2(size)) - 3;
}

int get_pool_index(Pool *p) {
    if (p == NULL || p->parent_allocator == NULL) return -1;

    Allocator *alloc = p->parent_allocator;

    Pool *start = alloc->custom_pools;            
    Pool *end   = start + MAX_CUSTOM_POOLS;      

    if (p >= start && p < end) {
        return (int)(p - start);
    }

    Pool *gen_start = alloc->generic_pools;
    Pool *gen_end   = gen_start + MAX_GENERIC_POOLS;

    if (p >= gen_start && p < gen_end) {
        return -2; 
    }

    return -1; 
}

void ensure_allocator_initialized() {
    if (global_allocator == NULL) {
        global_allocator = allocator_create();
    }
}
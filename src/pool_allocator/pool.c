/* 
__Arquitetura do Alocador de Memória__

* Backend Allocator:
    -> Gerencia as páginas de memória (4KB)
    -> Guarda uma free-list das páginas livres
    -> Caso necessário, pede mais memória ao SO
    -> Guarda os headers das Pools em um array de tamanho fixo

*/


#include "pool.h"
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>


void print_addr(void *ptr, char *label) {
    printf("%s Address: %ld\n", label, (uintptr_t)ptr);
}

bool is_power_of_two(size_t x) {
    return (x & (x-1)) == 0;
}

void *get_ptr_from_offset(void *start, size_t offset) {
    return (void*)((uintptr_t)start + offset);
}

int get_pool_index(Pool *p) {
    // 1. Segurança básica
    if (p == NULL || p->parent_allocator == NULL) return -1;

    Allocator *alloc = p->parent_allocator;

    // 2. Definir os limites do array 'custom_pools'
    Pool *start = alloc->custom_pools;             // &custom_pools[0]
    Pool *end   = start + MAX_CUSTOM_POOLS;        // Ponteiro logo após o último elemento

    // 3. Verificar se o ponteiro 'p' está DENTRO do array
    if (p >= start && p < end) {
        // Aritmética de Ponteiros: 
        // (p - start) retorna a diferença em ELEMENTOS, não em bytes.
        // O compilador faz o 'sizeof' automaticamente.
        return (int)(p - start);
    }

    // --- Opcional: Verificar se é uma Generic Pool ---
    Pool *gen_start = alloc->generic_pools;
    Pool *gen_end   = gen_start + MAX_GENERIC_POOLS;

    if (p >= gen_start && p < gen_end) {
        // Retornamos -2 ou imprimimos erro para diferenciar
        // printf("Aviso: Tentou pegar indice de Generic Pool em func de Custom Pool\n");
        return -2; 
    }

    // 4. Ponteiro não pertence a este alocador ou é inválido
    return -1; 
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



Allocator *allocator_create() {
    size_t heap_size = INITIAL_HEAP_SIZE; // 32 MiB
    void *heap_base = mmap(NULL, heap_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    mprotect(heap_base, PAGE_SIZE, PROT_READ | PROT_WRITE);
    
    Allocator *allocator = (Allocator*)heap_base;
    allocator->heap_start = heap_base;
    allocator->heap_capacity = heap_size;
    allocator->free_pages = NULL;

    // Init generic pools
    size_t pool_block_size = 8;
    size_t pool_alignment = 8;

    for (size_t i = 0; i < MAX_GENERIC_POOLS; i++) {
        Pool *current_pool = &allocator->generic_pools[i];
        if (i != 0) pool_alignment = DEFAULT_ALIGNMENT;
        current_pool->alignment = pool_alignment;
        current_pool->block_size = pool_block_size;
        current_pool->pool_size = 0;
        current_pool->free_list = NULL;
        current_pool->root_chunck = NULL;
        current_pool->curr_chunck = NULL;
        pool_block_size *= 2;
    }

    for (size_t i = 0; i < MAX_CUSTOM_POOLS; i++) {
        Pool *current_pool = &allocator->custom_pools[i];
        current_pool->alignment = DEFAULT_ALIGNMENT;
        current_pool->block_size = 0;
        current_pool->pool_size = 0;
        current_pool->free_list = NULL;
        current_pool->root_chunck = NULL;
        current_pool->curr_chunck = NULL;
    }

    size_t next_page_offset = (sizeof(Allocator) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    allocator->heap_offset = next_page_offset;

    return allocator;
}

Page_Header *backend_alloc_page(Allocator *allocator) {
    if (allocator->free_pages != NULL) {
        Page_Header *page = allocator->free_pages;
        allocator->free_pages = page->next;

        page->is_free = 0;
        page->offset = sizeof(Page_Header);
        return page;
    }

    if (allocator->heap_offset + PAGE_SIZE > allocator->heap_capacity) {
            fprintf(stderr, "Fatal: Out of Memory (Virtual Reservation Exhausted)\n");
            return NULL;
    }

    void *new_page_addr = (char*)allocator->heap_start + allocator->heap_offset;

    if (mprotect(new_page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE) != 0) {
        perror("mprotect failed in backend_alloc_page");
        return NULL;
    }

    allocator->heap_offset += PAGE_SIZE;

    Page_Header *page = (Page_Header*)new_page_addr;
    page->next = NULL;
    page->is_free = 0;
    page->offset = sizeof(Page_Header);

    return page;
}

Chunck *get_chunck(Pool *pool) {
    Page_Header *new_page = backend_alloc_page(pool->parent_allocator);

    Chunck *new_chunck = (Chunck*)((uintptr_t)new_page + sizeof(Page_Header));
    new_chunck->next = pool->curr_chunck;
    new_chunck->parent_pool = pool;

    if (pool->root_chunck == NULL) { 
        pool->root_chunck = new_chunck;
        pool->curr_chunck = new_chunck;
    } else {
        pool->curr_chunck = new_chunck;
    }

    new_page->offset += sizeof(Chunck);

    return (void*)new_chunck;
}

Block *get_free_blocks(Chunck *chunck) {
    assert(chunck != NULL && "Invalid chunck pointer passed as argument");

    Block *free_list_head = (Block*)((uintptr_t)chunck + sizeof(Chunck));
    Block *curr_block = free_list_head;

    size_t header_size = sizeof(Page_Header) + sizeof(Chunck);
    size_t block_size = chunck->parent_pool->block_size;
    size_t num_blocks = (PAGE_SIZE - header_size) / block_size;

    for (size_t i = 0; i < num_blocks - 1; i++) {
        curr_block->next = (Block*)((uintptr_t)curr_block + block_size);
        curr_block = curr_block->next;
    }
    curr_block->next = NULL;

    return free_list_head;
}

void pool_get_memory(Pool *pool, size_t size) {
    assert((size > 0) && "Invalid size requested");

    size_t needed_chuncks = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    while (needed_chuncks > 0) {
        Chunck *new_chunck = get_chunck(pool);
        Block *new_free_list = get_free_blocks(new_chunck);

        if (pool->free_list == NULL) pool->free_list = new_free_list;
        else pool->free_list->next = new_free_list;

        needed_chuncks--;
    }
}

Pool *pool_create(Allocator *allocator, size_t block_size, size_t block_count, size_t alignment) {
    // Search for free custom pool
    size_t bs = align_size(block_size, alignment);
    size_t alloc_size = bs * block_count;

    for (size_t i = 0; i < MAX_CUSTOM_POOLS; i++) {
        if (allocator->custom_pools[i].pool_size == 0) {
            // Achou slot livre
            Pool *pool = &allocator->custom_pools[i];
            
            // ... (Configura pool_size, block_size, etc) ...
            
            // IMPORTANTE: Configure o pai aqui
            pool->parent_allocator = allocator; 
            pool->pool_size = alloc_size;
            pool->alignment = alignment;
            pool->block_size = bs;
            pool->parent_allocator = allocator;

            pool_get_memory(pool, alloc_size);
            return pool;
        }
    }

    perror("Error: Out of custom pools!\n");
    return NULL;
}

void *pool_alloc(Pool *p) {
    if (p->free_list == NULL) {
        pool_get_memory(p, PAGE_SIZE);

        if (p->free_list == NULL) return NULL;
    }

    Block *result = p->free_list;
    p->free_list = result->next;
    
    return (void*)result;
}

void pool_free_block(Pool *p, void *ptr) {
    Block *block = (Block*)ptr;
    block->next = p->free_list;
    p->free_list = block;
}

void pool_free(void *ptr) {
    uintptr_t mask = ~(PAGE_SIZE - 1);
    uintptr_t page_start = (uintptr_t)ptr & mask;

    Chunck *retrived_chunck = (Chunck*)(page_start + sizeof(Page_Header));
    Pool *p = retrived_chunck->parent_pool;

    pool_free_block(p, ptr);
}

void pool_destroy(Pool *pool) {
    if (pool == NULL) return;

    Chunck *temp = pool->root_chunck;
    while (temp) {
        Page_Header *page = (Page_Header*)((uintptr_t)temp - sizeof(Page_Header));

        page->is_free = 1;
        page->offset = 0;
        page->next = pool->parent_allocator->free_pages;
        pool->parent_allocator->free_pages = page;

        temp = temp->next;
    }
    printf("Pool destroyed [%d]\n", get_pool_index(pool));

    pool->pool_size = 0;
    pool->free_list = NULL;
    pool->parent_allocator = NULL;
    pool->root_chunck = NULL;
    pool->curr_chunck = NULL;

}
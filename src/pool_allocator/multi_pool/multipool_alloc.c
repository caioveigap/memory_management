#include "multipool_alloc.h"
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

/*
TODO: Suportar blocos com menos de 4 bytes usando um offset ao invés de ponteiros.
TODO: Criar função pool_grow(), para criar um novo chunck e linkar com o chunck atual
 */

void print_addr(void *ptr, char *label) {
    printf("%s Address: %ld\n", label, (uintptr_t)ptr);
}

void debug_print_page(Page_Allocator *page) {
    if (!page) { printf("Page is NULL\n"); return; }

    printf("\n=== PAGE DUMP (Cap: %zu | Used: %zu) ===\n", page->capacity, page->offset);
    printf("Memory Addr: %p\n", (void*)page);
    
    unsigned char *mem = page->temp_memory;
    size_t limit = page->capacity;
    
    // Imprime cabeçalho
    printf("Offset  | Bytes (Hex)                                     | ASCII\n");
    printf("--------|-------------------------------------------------|----------------\n");

    for (size_t i = 0; i < limit; i += 16) {
        // Para não poluir, se a página for gigante (ex: 1MB), 
        // pare de imprimir se passar muito do offset usado.
        if (i > page->offset + 32 && i > 64) {
            printf("... (restante da memória não utilizada) ...\n");
            break;
        }

        printf("%04zX    | ", i);

        // Hex
        for (size_t j = 0; j < 16; j++) {
            if (i + j < limit) {
                // Destaque visual: Se o byte está na área usada, imprime normal.
                // Se está na área livre, imprime mais apagado (se o terminal suportar)
                // Aqui apenas diferenciamos logicamente:
                printf("%02X ", mem[i + j]);
            } else {
                printf("   ");
            }
        }
        
        printf("| ");

        // ASCII
        for (size_t j = 0; j < 16; j++) {
            if (i + j < limit) {
                unsigned char c = mem[i + j];
                printf("%c", isprint(c) ? c : '.');
            }
        }
        printf("\n");
    }
    printf("-------------------------------------------------------------------\n");
}

bool is_power_of_two(size_t x) {
    return (x & (x-1)) == 0;
}

void *get_ptr_from_offset(void *start, size_t offset) {
    return (void*)((uintptr_t)start + offset);
}

size_t align_block_size(size_t size, size_t alignment) {
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

void *request_memory(Allocator *alloc, size_t size) {
    size_t num_pages = (size + PAGE_SIZE - 1) /  PAGE_SIZE;
    size_t alloc_size = num_pages * PAGE_SIZE;
    
    void *page_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Insere página na lista de páginas
    alloc->num_pages += num_pages;
    Page_Allocator *page_alloc = (Page_Allocator*)page_ptr;
    page_alloc->capacity = alloc_size;
    page_alloc->offset = sizeof(Page_Allocator);
    page_alloc->next = alloc->pages;
    alloc->pages = page_alloc;

    page_ptr = (void*)((uintptr_t)page_ptr + (uintptr_t)(sizeof(Page_Allocator)));
    return page_ptr;
}
/* 
TODO: Fix pointer arithmetic and pool allocation
*/
Allocator *allocator_create() {
    void *raw_mem = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Allocator *alloc = (Allocator*)raw_mem;
    Page_Allocator *root_page = (Page_Allocator*)raw_mem;
    root_page->capacity = PAGE_SIZE;
    root_page->offset = sizeof(Page_Allocator);
    root_page->next = NULL;
    root_page->temp_memory = raw_mem;

    Allocator *alloc = (Allocator*)get_ptr_from_offset(root_page, sizeof(Page_Allocator));
    alloc->num_pages = 0;
    alloc->num_pools = 8;
    alloc->pages = NULL;

    void *pools_start = (void*)((char*)alloc + sizeof(Allocator));
    alloc->pools = align_ptr(pools_start, DEFAULT_ALIGNMENT);
    // Criar pools genéricas
    /* 4, 8, 16, 32, 64, 128, 256, 512 */

    alloc->num_pools = 8;
    size_t current_block_size = 4;

    for (int i = 0; i < alloc->num_pools; i++) {
        Pool *p = &alloc->pools[i];

        p->block_size = current_block_size;

        if (current_block_size < DEFAULT_ALIGNMENT) {
            p->alignment = current_block_size;
        } else {
            p->alignment = DEFAULT_ALIGNMENT;
        }

        p->blocks_per_chunck = PAGE_SIZE / p->block_size;
        p->chuncks = NULL;
        p->free_list = NULL;

        current_block_size *= 2;
    }

    uintptr_t pools_end = ((uintptr_t)pools_start + (sizeof(Pool) * POOL_ALLOCATION_LIMIT));
    void *free_start = align_ptr((void*)pools_end, DEFAULT_ALIGNMENT);
    root_page->offset = ((uintptr_t)free_start - (uintptr_t)raw_mem);

    return alloc;
}

Pool *pool_create(Allocator *alloc, size_t block_size, size_t alignment) {
    Pool *pool = &alloc->pools[alloc->num_pools];
    alloc->num_pools++;

    pool->block_size = align_block_size(block_size, alignment);
    pool->alignment = alignment;
    pool->blocks_per_chunck = (PAGE_SIZE - sizeof(Pool_Chunck) - pool->alignment) / pool->block_size;
    
    void *chunck_memory = request_memory(alloc, PAGE_SIZE);
    pool->chuncks = (Pool_Chunck*)chunck_memory;
    pool->chuncks->next = NULL;
    
    unsigned char *block_start = (unsigned char*)chunck_memory + sizeof(Pool_Chunck);
    block_start = align_ptr(block_start, pool->alignment);

    Pool_Block *current = (Pool_Block*)block_start;
    pool->free_list = current; //Começo dos blocos

    for (size_t i = 0; i < pool->blocks_per_chunck - 1; i++) {
        current->next = (Pool_Block*)((unsigned char*)current + pool->block_size);
        current = current->next;
    }

    current->next = NULL;

    return pool;
}

void *palloc() {
    
}

void *pool_alloc(Pool *pool) {
    Pool_Block *block = pool->free_list;
    pool->free_list = block->next;

    return (void*)block;
}
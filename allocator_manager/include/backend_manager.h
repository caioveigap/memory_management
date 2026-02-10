#pragma once
#include <stddef.h>
#include "utilities.h"


#define PAGE_SIZE 4096
#define RESERVED_MEMORY_REGION_SIZE (size_t)(1024 * 1024 * 2)
#define MAX_BIN_ORDER 5 // Tamanho máximo da Bin é de 256 KB
#define MAX_DEFAULT_ALLOCATION_SIZE (PAGE_SIZE * (1 << MAX_BIN_ORDER))
#define HUGE_MAGIC_NUMBER 0xF22CAA33CE
#define DEFAULT_ALIGNMENT (2 * sizeof(void*))

#define REQUEST_SIZE_FROM_ORDER(order) ((1 << (order)) * PAGE_SIZE)


typedef enum Page_Owner {
    OWNER_NONE,
    OWNER_ARENA,
    OWNER_POOL,
    OWNER_HEAP,
    OWNER_DEBUG
} Page_Owner;

typedef struct Page_Descriptor {
    u8 flags;
    u8 order;
    Page_Owner owner_id;    
    
    void *zone_header;
    
    struct Page_Descriptor *next;
    struct Page_Descriptor *prev;
} Page_Descriptor;

void backend_init(size_t total_memory_size);
void *backend_alloc(size_t size, Page_Owner owner);
void backend_free(void *ptr);

Page_Descriptor *get_descriptor(void *ptr);
void *get_address(Page_Descriptor *node);
void debug_print_backend_manager_stats();
u32 get_order(size_t size);
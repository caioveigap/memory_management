#pragma once
#include <stddef.h>

#define PAGE_SIZE 4096
#define RESERVED_MEMORY_REGION_SIZE 67108864 // 32 MB ou 16384 páginas virtuais
#define MAX_BIN_ORDER 8 // Tamanho máximo da Bin é de 1 MB
#define MAX_DEFAULT_ALLOCATION_SIZE 262144 // 256 KB
#define HUGE_MAGIC_NUMBER 0xF22CAA33CE

typedef enum Page_Owner {
    OWNER_NONE,
    OWNER_ARENA,
    OWNER_POOL,
    OWNER_HEAP,
    OWNER_DEBUG
} Page_Owner;


void backend_init(size_t total_memory_size);
void *backend_alloc(size_t size, Page_Owner owner);
void backend_free(void *ptr);

void debug_print_bins();
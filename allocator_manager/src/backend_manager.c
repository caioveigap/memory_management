#include <sys/mman.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


#define PAGE_SIZE 4096
#define RESERVED_MEMORY_REGION_SIZE 268435456 // 256 MB ou 65536 páginas virtuais
#define MAX_BIN_ORDER 8 // Tamanho máximo da Bin é de 1 MB

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;


// ==========================
//  FLAGS DO PAGE DESCRIPTOR
// ==========================

// #define PAGE_ALLOCATED 0x0000
// #define PAGE_UNMAPED 0x000

#define PAGE_FREE 0x01
#define PAGE_MMAPED 0x02
#define PAGE_HUGE_ALLOCATION 0x04
#define PAGE_HEAD 0x08


// ==========================
//  ESTRUTURAS PRINCIPAIS
// ==========================

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
    Page_Owner owner_type;

    struct Page_Descriptor *next;
    struct Page_Descriptor *previous;
} Page_Descriptor;

typedef struct Page_Bin {
    Page_Descriptor *free_list;
} Page_Bin;

typedef struct Backend_Page_Manager {
    void *memory_start;
    size_t total_pages;
    size_t page_offset_index;

    Page_Descriptor *page_map;
    Page_Bin bins[MAX_BIN_ORDER+1];
    // debug info ...
} Backend_Page_Manager;


// VARIÁVEIS GLOBAIS
Backend_Page_Manager *backend_manager;

// ==========================
//  FUNÇÕES AUXILIARES
// ==========================

u32 fast_log2(u32 n) {
    if (n == 0) return 0; // Prevenção de erro
    
    // __builtin_clz conta os "Leading Zeros" (zeros à esquerda)
    // sizeof(unsigned int) * 8 geralmente é 32
    return (sizeof(unsigned int) * 8) - 1 - __builtin_clz(n);
}

u32 round_up_pow2(u32 n) {
    // __builtin_clz retorna o número de zeros à esquerda.
    // Em um int de 32 bits, subtraímos isso de 32 para achar a posição do bit.
    return 1 << (32 - __builtin_clz(n - 1));
}

u32 get_num_pages_from_size(size_t size) {
    size_t requested_size = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    return round_up_pow2(size);
}

u32 get_order(size_t size) {
    u32 num_pages = get_num_pages_from_size(size);

    if (num_pages == 0) return 0; // Prevenção de erro
    
    return fast_log2(num_pages);
}

Page_Descriptor *get_descriptor(void *ptr) {
    u32 page_idx = ((char*)ptr - (char*)backend_manager->memory_start) / PAGE_SIZE;
    return &backend_manager->page_map[page_idx];
}

void *get_address(Page_Descriptor *desc) {
    u32 page_idx = desc - backend_manager->page_map;
    return (void*)((char*)backend_manager->memory_start + (page_idx * PAGE_SIZE));
}

void bin_push(Page_Bin *bin, Page_Descriptor *desc) {
    if (bin->free_list == NULL) {
        desc->previous = NULL;
        desc->next = NULL;
        bin->free_list = desc;
        return;
    }

    desc->previous = NULL;
    desc->next = bin->free_list;
    bin->free_list->previous = desc;
    bin->free_list = desc;

    return;
}

// ==========================
//  FUNÇÕES PRINCIPAIS
// ==========================

void backend_init(size_t total_memory_size) {
    size_t total_pages = total_memory_size / PAGE_SIZE;
    size_t metadata_size = sizeof(Backend_Page_Manager) + (sizeof(Page_Descriptor) * total_pages);

    // Reservar memória do SO
    void *raw_mem = mmap(NULL, total_memory_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Alocar 'metadata_size' bytes da memória reservada
    mprotect(raw_mem, metadata_size, PROT_READ | PROT_WRITE);

    size_t metadata_pages = (metadata_size + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t available_pages = total_pages - metadata_pages;

    // Inicializar gerenciador de páginas
    backend_manager = (Backend_Page_Manager*)raw_mem;
    backend_manager->total_pages = available_pages;
    backend_manager->memory_start = (void*)((char*)raw_mem + (metadata_pages * PAGE_SIZE));
    backend_manager->page_map = (Page_Descriptor*)((char*)raw_mem + sizeof(Backend_Page_Manager));
    backend_manager->page_offset_index = 0;
    
    // Inicializar (0) o mapa dos descritores de páginas do gerenciador
    for (size_t i = 0; i < available_pages; i++) {
        backend_manager->page_map[i] = (Page_Descriptor){0};
    }

    // Inicializar (NULL) as bins de áreas livres
    for (size_t i = 0; i < MAX_BIN_ORDER; i++) {
        backend_manager->bins[i].free_list = NULL;
    }

    // Alocar um bloco de maior ordem

}

/*
Aloca memória virgem, da reserva, para a Bin de maior ordem 
 */
int backend_request_memory() {
    size_t bin_size = 1 << (MAX_BIN_ORDER - 1);
    size_t alloc_size = bin_size * PAGE_SIZE;

    if (backend_manager->page_offset_index + bin_size > backend_manager->total_pages) {
        fprintf(stderr, "Error: Out Of Memory");
        return 0;
    }

    Page_Descriptor *head = &backend_manager->page_map[backend_manager->page_offset_index];
    void *page_start = get_address(head);
    mprotect(page_start, alloc_size, PROT_READ | PROT_WRITE);

    head->flags = PAGE_FREE | PAGE_MMAPED | PAGE_HEAD;
    head->order = MAX_BIN_ORDER;
    // head->owner_type = OWNER_NONE;
    head->owner_type = OWNER_DEBUG;

    bin_push(&backend_manager->bins[MAX_BIN_ORDER], head);
    backend_manager->page_offset_index += bin_size;

    return 1;
}

// void *backend_alloc_pages() {

// }

int main() {

    backend_init(RESERVED_MEMORY_REGION_SIZE);
    int r = backend_request_memory();

    Page_Bin b = backend_manager->bins[MAX_BIN_ORDER];

    if (b.free_list != NULL) {
        printf("Order: %u\n", b.free_list->order);
        printf("Owner: %u\n", b.free_list->owner_type);
    }

    return 0;
}
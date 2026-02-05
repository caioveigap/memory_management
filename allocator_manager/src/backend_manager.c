#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#include "../include/backend_manager.h"
#include "../include/utilities.h"


#define PAGE_FREE 0x01
#define PAGE_MMAPED 0x02
#define PAGE_HUGE_ALLOCATION 0x04
#define PAGE_HEAD 0x08


// ==========================
//  ESTRUTURAS PRINCIPAIS
// ==========================

typedef struct Pool_Chunck Pool_Chunck;
typedef struct Heap_Chunck Heap_Chunck;


typedef struct Huge_Allocation_Metadata {
    size_t total_size;
    u64 magic_number;
    Page_Owner owner;

    // debug info
    struct Huge_Allocation_Metadata *next;
    struct Huge_Allocation_Metadata *prev;
} Huge_Allocation_Metadata;


typedef struct Page_Bin {
    Page_Descriptor *free_list;
} Page_Bin;

typedef struct Backend_Page_Manager {
    void *memory_start;
    size_t total_pages;
    size_t used_pages;
    size_t page_offset_index;

    Page_Descriptor *page_map;
    Page_Bin bins[MAX_BIN_ORDER+1];

    // debug info
    Huge_Allocation_Metadata *huge_allocation_list;
} Backend_Page_Manager;



// DECLARAÇÕES
u32 get_num_pages_from_size(size_t size);
void backend_set_zone(Page_Descriptor *head, Page_Owner owner);
u32 get_order(size_t size);
Page_Descriptor *get_descriptor(void *ptr);
void *get_address(Page_Descriptor *node);
void bin_push(Page_Bin *bin, Page_Descriptor *node);
void bin_remove(Page_Bin *bin, Page_Descriptor *node);
Page_Descriptor *bin_pop(Page_Bin *bin);
bool is_huge_allocation(void *ptr);

Backend_Page_Manager *backend_manager;

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
    backend_manager->used_pages = 0;
    
    // debug section start
    backend_manager->huge_allocation_list = NULL;
    // debug section end
    
    // Inicializar (0) o mapa dos descritores de páginas do gerenciador
    for (size_t i = 0; i < available_pages; i++) {
        backend_manager->page_map[i] = (Page_Descriptor){0};
    }

    // Inicializar (NULL) as bins de áreas livres
    for (size_t i = 0; i <= MAX_BIN_ORDER; i++) {
        backend_manager->bins[i].free_list = NULL;
    }
}

/*
Aloca memória virgem, da reserva, para a Bin de maior ordem 
 */
int backend_request_memory() {
    size_t bin_size = 1 << MAX_BIN_ORDER;
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
    // head->owner_id = OWNER_NONE;
    head->owner_id = OWNER_DEBUG;

    bin_push(&backend_manager->bins[MAX_BIN_ORDER], head);
    backend_manager->page_offset_index += bin_size;

    return 1;
}

void *backend_alloc_huge(size_t size, Page_Owner owner) {
    size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t total_size = (num_pages + 1) * PAGE_SIZE; // +1 Página para o header

    void *mem_ptr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mem_ptr == MAP_FAILED) {
        fprintf(stderr, "Huge Allocation Failed");
        return NULL;
    }

    Huge_Allocation_Metadata *meta = (Huge_Allocation_Metadata*)mem_ptr;
    meta->total_size = total_size;
    meta->magic_number = HUGE_MAGIC_NUMBER;
    meta->owner = owner;

    // debug section start
    if (backend_manager->huge_allocation_list == NULL) {
        meta->next = NULL;
        meta->prev = NULL;
        backend_manager->huge_allocation_list = meta;
    } else {
        meta->next = backend_manager->huge_allocation_list;
        backend_manager->huge_allocation_list->prev = meta;
        meta->prev = NULL;
        backend_manager->huge_allocation_list = meta;
    }
    // debug section end

    return (u8*)mem_ptr + PAGE_SIZE;
}


void backend_free_huge_allocation(void *ptr) {
    Huge_Allocation_Metadata *meta = (Huge_Allocation_Metadata*)((u8*)ptr - PAGE_SIZE);

    if (meta->magic_number != HUGE_MAGIC_NUMBER) {
        fprintf(stderr, "Error: Invalid huge block free request");
        return;
    }

    // debug section start
    if (backend_manager->huge_allocation_list == meta) {
        backend_manager->huge_allocation_list = meta->next;
        if (meta->next) {
            meta->next->prev = meta->prev;
        } 
        
    } else {
        if (meta->prev) {
            meta->prev->next = meta->next;
        }
        if (meta->next) {
            meta->next->prev = meta->prev;
        }
    }
    // debug section end
    

    munmap(ptr, meta->total_size);
}

void *backend_alloc(size_t size, Page_Owner owner) {
    if (backend_manager == NULL) {
        fprintf(stderr, "Error: Backend Manager not initialized [backend_init()]\n");
        return NULL;
    }

    size_t aligned_size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (aligned_size > MAX_DEFAULT_ALLOCATION_SIZE) {
        return backend_alloc_huge(aligned_size, owner);
    }
 

    int order = get_order(aligned_size);

    assert((order >= 0 && order <= MAX_BIN_ORDER) && "Invalid target order");
    u8 k = order;

    while (k <= MAX_BIN_ORDER) {
        if (backend_manager->bins[k].free_list != NULL) break;
        k++;
    }

    if (k > MAX_BIN_ORDER) {
        if (!backend_request_memory()) {
            return NULL;
        }
        k = MAX_BIN_ORDER;
    }

    Page_Descriptor *block = bin_pop(&backend_manager->bins[k]);

    while (k > order) {
        k--;

        size_t half_size = 1 << k;

        Page_Descriptor *buddy = block + half_size;

        buddy->flags = PAGE_FREE | PAGE_HEAD;
        buddy->order = k;
        buddy->owner_id = OWNER_NONE;

        bin_push(&backend_manager->bins[k], buddy);
        block->order = k;
    }

    block->flags &= ~PAGE_FREE;
    block->flags |= PAGE_HEAD;
    block->owner_id = owner;

    backend_set_zone(block, owner);
    backend_manager->used_pages += (1 << block->order);
    return get_address(block);
}

void backend_free(void *ptr) {
    if (is_huge_allocation(ptr)) {
        backend_free_huge_allocation(ptr);
        return;
    }

    Page_Descriptor *block = get_descriptor(ptr);

    // Sanity Check: Double Free
    if (block->flags & PAGE_FREE) {
        // fprintf(stderr, "Double Free!\n");
        return;
    }

    backend_manager->used_pages -= (1 << block->order);

    // Marca o bloco atual como livre para começar a subir a cascata
    block->flags |= PAGE_FREE;
    block->flags |= PAGE_HEAD; // Garante que é uma cabeça válida
    block->owner_id = OWNER_NONE;

    int k = block->order;

    while (k < MAX_BIN_ORDER) {
        size_t index = block - backend_manager->page_map;
        size_t buddy_index = index ^ (1 << k);

        // 1. PROTEÇÃO DE BORDA (Faltava no seu snippet)
        if (buddy_index >= backend_manager->total_pages) break;

        Page_Descriptor *buddy = &backend_manager->page_map[buddy_index];

        // 2. CORREÇÃO CRÍTICA: Operador & (Bitwise)
        // Verificamos se é Livre, se é Cabeça, e se tem a mesma Ordem.
        if ((buddy->flags & PAGE_FREE) && 
            (buddy->flags & PAGE_HEAD) && 
            (buddy->order == k)) {
            
            // Remove o vizinho da lista para fundir
            bin_remove(&backend_manager->bins[k], buddy);

            if (buddy_index < index) {
                block = buddy;
                // O bloco da direita perde o status de HEAD e FREE pois foi engolido
                // (Opcional, mas ajuda no debug limpar as flags do bloco engolido)
                Page_Descriptor *engolido = &backend_manager->page_map[index];
                engolido->flags = 0; 
                engolido->order = 0; 
            } else {
                 // Se o buddy foi engolido (ele estava à direita)
                 buddy->flags = 0;
                 buddy->order = 0;
            }

            k++;
            block->order = k;
            block->flags |= PAGE_HEAD; // Reafirma que o novo blocão é HEAD
        } else {
            break; // Não dá pra fundir
        }
    }

    // Insere o bloco final (agora maior) na lista
    bin_push(&backend_manager->bins[k], block);
    // backend_manager
}

void backend_set_zone(Page_Descriptor *head, Page_Owner owner) {
    size_t page_count = 1 << (head->order);

    void *zone_start_addr = get_address(head);
    
    head->zone_header = zone_start_addr; 


    for (size_t i = 1; i < page_count; i++) {    
        Page_Descriptor *tail = &head[i];

        tail->owner_id = owner;
        tail->zone_header = zone_start_addr;
        tail->flags &= ~(PAGE_FREE | PAGE_HEAD); 
        tail->order = head->order; 
    }
}

// ==========================
//  FUNÇÕES AUXILIARES
// ==========================


u32 get_num_pages_from_size(size_t size) {
    size_t requested_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    return round_up_pow2(requested_pages);
}

u32 get_order(size_t size) {
    u32 num_pages = get_num_pages_from_size(size);

    if (num_pages == 0) return 0; // Prevenção de erro
    
    return fast_log2(num_pages);
}

Page_Descriptor *get_descriptor(void *ptr) {
    u32 page_idx = ((char*)ptr - (char*)backend_manager->memory_start) / PAGE_SIZE;
    if (page_idx >= backend_manager->total_pages) {
        fprintf(stderr, "Error: Pointer out of bounds\n");
        return NULL;
    }
    return &backend_manager->page_map[page_idx];
}

void *get_address(Page_Descriptor *node) {
    u32 page_idx = node - backend_manager->page_map;
    return (void*)((char*)backend_manager->memory_start + (page_idx * PAGE_SIZE));
}

void bin_push(Page_Bin *bin, Page_Descriptor *node) {
    if (bin->free_list == NULL) {
        node->prev = NULL;
        node->next = NULL;
        bin->free_list = node;
        return;
    }

    node->prev = NULL;
    node->next = bin->free_list;
    bin->free_list->prev = node;
    bin->free_list = node;

    return;
}

void bin_remove(Page_Bin *bin, Page_Descriptor *node) {
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        bin->free_list = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }

    node->prev = NULL;
    node->next = NULL;
}

Page_Descriptor *bin_pop(Page_Bin *bin) {
    if (bin->free_list == NULL) return NULL;

    Page_Descriptor *node = bin->free_list;
    bin_remove(bin, node);
    return node;
}

bool is_huge_allocation(void *ptr) {
    void *start = backend_manager->memory_start;
    void *end = (u8*)start + (PAGE_SIZE * backend_manager->total_pages);  

    return ((ptr >= start) && (ptr < end)) ? false : true;
}

// ==========================
//  FUNÇÕES DEBUG
// ==========================


// Helper para traduzir o enum Owner para String
const char* debug_get_owner_name(u8 owner) {
    switch(owner) {
        case OWNER_NONE:  return "NONE";
        case OWNER_ARENA: return "ARENA";
        case OWNER_POOL:  return "POOL";
        case OWNER_HEAP:  return "HEAP";
        case OWNER_DEBUG: return "DEBUG";
        default:          return "UNKNOWN";
    }
}

// Helper para formatar tamanho (KB/MB)
void debug_format_size(size_t bytes, char *buffer) {
    if (bytes >= 1024 * 1024) {
        sprintf(buffer, "%zu MB", bytes / (1024 * 1024));
    } else if (bytes >= 1024) {
        sprintf(buffer, "%zu KB", bytes / 1024);
    } else {
        sprintf(buffer, "%zu B", bytes);
    }
}


void debug_print_bins() {
    if (backend_manager == NULL) {
        printf("[DEBUG] Backend Manager não inicializado.\n");
        return;
    }

    printf("\n");
    printf("=====================================================\n");
    printf("                   BINS STATES                       \n");
    printf("=====================================================\n");
    printf(" %-10s | %-10s | %-35s \n", "ORDER", "BLOCK SIZE", "CONTENT (HEAD ADDRESSES)");
    printf("---------------------------------------------------------------\n");

    size_t total_free_memory = 0;

    // Itera da maior ordem para a menor (visualização mais lógica)
    for (int i = MAX_BIN_ORDER; i >= 0; i--) {
        Page_Bin *bin = &backend_manager->bins[i];
        
        // Calcula o tamanho do bloco nesta ordem
        size_t block_size = (1 << i) * PAGE_SIZE;
        char size_str[16];
        debug_format_size(block_size, size_str);

        printf(" [%d]        | %-10s | ", i, size_str);

        if (bin->free_list == NULL) {
            printf("(Empty)\n");
            continue;
        }

        // Percorre a lista encadeada
        Page_Descriptor *curr = bin->free_list;
        int count = 0;
        
        while (curr != NULL) {
            void *addr = get_address(curr); // Usa sua função auxiliar
            
            // Formata flags para visualização compacta [FH..]
            // F=Free, H=Head, M=Mmap, G=Huge
            char flags[5] = "....";
            if (curr->flags & PAGE_FREE) flags[0] = 'F';
            if (curr->flags & PAGE_HEAD) flags[1] = 'H';
            if (curr->flags & PAGE_MMAPED) flags[2] = 'M';
            if (curr->flags & PAGE_HUGE_ALLOCATION) flags[3] = 'G';

            // Imprime o bloco
            // Ex: [0x7f...000 (DEBUG)] ->
            if (count > 0) printf(" -> ");
            printf("[%p (%s)]", addr, debug_get_owner_name(curr->owner_id));

            total_free_memory += block_size;
            curr = curr->next;
            count++;

            // Quebra de linha se a lista for muito longa
            if (count % 3 == 0 && curr != NULL) {
                printf("\n %-10s | %-10s | ", "", "");
            }
        }
        printf("\n");
    }
    
    char total_str[16];
    debug_format_size(total_free_memory, total_str);
    printf("---------------------------------------------------------------\n");
    printf(" TOTAL FREE MEMORY: %s\n", total_str);
    printf("===============================================================\n\n");
}

void debug_print_huge_allocation_list() {
    int n = 0;

    Huge_Allocation_Metadata *header = backend_manager->huge_allocation_list;
    printf("----------------------------------------------\n");
    printf("HUGE ALLOCATION LIST:       |  SIZE  |  OWNER  |\n");
    printf("----------------------------------------------\n");

    while (header != NULL) {
        printf("ALLOCATION [%p]:  %ld | %s\n", header, header->total_size, debug_get_owner_name(header->owner));
        n++;
        header = header->next;
    };
    printf("\n");
}

void debug_print_backend_manager_stats() {
    printf("\n======BACKEND MANAGER STATS======\n");

    printf("Reserved Memory Capacity: %ld [%ld pages]\n", RESERVED_MEMORY_REGION_SIZE, (RESERVED_MEMORY_REGION_SIZE/PAGE_SIZE));
    printf("Memory Start Address: %p\n", backend_manager->memory_start);
    printf("Total Pages: %ld\n", backend_manager->total_pages);
    printf("Metadata Size: %ld [%ld pages]\n\n", (RESERVED_MEMORY_REGION_SIZE - (backend_manager->total_pages * PAGE_SIZE)), ((RESERVED_MEMORY_REGION_SIZE/PAGE_SIZE) - backend_manager->total_pages));
    debug_print_huge_allocation_list();
    debug_print_bins();
}
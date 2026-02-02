#include "heap.h"
#include <stdio.h>
#include <stdint.h>


// ==========================
//  ESTRUTURAS PRINCIPAIS
// ==========================

typedef struct Free_Chunck Free_Chunck;
typedef struct Allocation_Header Allocation_Header;

struct Heap_Allocator {
    void *data;
    size_t capacity;
    size_t used;

    Free_Chunck *free_list;
};

struct Allocation_Header
{
    size_t size;
    size_t padding;
};

struct Free_Chunck {
    Free_Chunck *next;
    Free_Chunck *previous;
    size_t size;
};

// =============
//  DEBUG
// =============

typedef struct Debug_Alloced_List {
    Allocation_Header *used_chuncks[512];
    size_t count;
} Debug_Alloced_List;


Debug_Alloced_List debug_list = {.used_chuncks = {0}, .count = 0};


// ==========================
//  FUNÇÕES DEBUG
// ==========================


void print_allocated_chunck_list() {
    for (size_t i = 0; i < debug_list.count; i++) {
        Allocation_Header *h = debug_list.used_chuncks[i];

        printf("\n_________________");
        printf("ALLOCATED BLOCK\n");

        printf("SIZE: %ld\n", h->size);
        printf("START ADDR [%p] -------- [%p] END ADDR\n", (void*)h, (void*)((char*)h + h->size));
        printf("-----------------\n");
    }
}

void print_free_chunck_list(Heap_Allocator *allocator) {
    Free_Chunck *temp = allocator->free_list;

    while (temp) {
        printf("\n_________________");
        printf("FREE BLOCK\n");

        printf("SIZE: %ld\n", temp->size);
        printf("START ADDR [%p] -------- [%p] END ADDR\n", (void*)temp, (void*)((char*)temp + temp->size));
        printf("-----------------\n");

        temp = temp->next;
    }
}

void debug_remove_chunck(Allocation_Header *target) {
    for (size_t i = 0; i < debug_list.count; i++) {
        if (debug_list.used_chuncks[i] == target) {
            // Troca o atual pelo último da lista
            debug_list.used_chuncks[i] = debug_list.used_chuncks[debug_list.count - 1];
            debug_list.count--;
            return;
        }
    }
    printf("ERRO DEBUG: Tentou liberar bloco nao rastreado: %p\n", target);
}

// ==========================
//  FUNÇÕES AUXILIARES
// ==========================

size_t calculate_padding(uintptr_t ptr, size_t header_size, size_t alignment) {
    uintptr_t modulo = ptr % alignment;
    size_t padding = 0;

    if (modulo != 0) {
        padding = alignment - modulo;
    }

    size_t needed_space = header_size;

    if (padding < needed_space) {
        needed_space -= padding;

        if (needed_space % alignment != 0) {
            padding += alignment * (1 + (needed_space/alignment));
        } else {
            padding += alignment * (needed_space/alignment);
        }
    }

    return padding;
}

void free_list_insert_chunck(Heap_Allocator *allocator, Free_Chunck *previous_chunck, Free_Chunck *new_chunck) {
    if (previous_chunck == NULL) {
        new_chunck->next = allocator->free_list;
        new_chunck->previous = NULL;

        if (allocator->free_list != NULL) {
            // new_chunck->next = allocator->free_list;
            allocator->free_list->previous = new_chunck;
        }
        allocator->free_list = new_chunck;
    } else {
        new_chunck->next = previous_chunck->next;
        new_chunck->previous = previous_chunck;

        if (previous_chunck->next != NULL) {
            previous_chunck->next->previous = new_chunck;
        }
        previous_chunck->next = new_chunck;
    }
}

void free_list_remove_chunck(Heap_Allocator *allocator, Free_Chunck *del_chunck) {
    if (del_chunck->previous == NULL) {
        allocator->free_list = del_chunck->next;
    } else {
        del_chunck->previous->next = del_chunck->next;
    }

    if (del_chunck->next != NULL) {
        del_chunck->next->previous = del_chunck->previous;
    }
}

Free_Chunck *find_best_chunck(Free_Chunck *free_list, size_t requested_size, size_t alignment) {
    Free_Chunck *curr_chunck = free_list;
    Free_Chunck *best_chunck = NULL;
    size_t best_diff = ~(size_t)0;
    size_t padding, free_data_size, diff;

    while (curr_chunck != NULL) {
        padding = calculate_padding((uintptr_t)curr_chunck, sizeof(Allocation_Header), DEFAULT_ALIGNMENT);
        free_data_size = curr_chunck->size - padding;
        
        if ((free_data_size >= requested_size)) {
            diff = free_data_size - requested_size;

            if (diff < best_diff) {
                best_chunck = curr_chunck;
                best_diff = diff;
            }
        }

        curr_chunck = curr_chunck->next;
    }

    return best_chunck;
}

void free_list_coalescence_chunck(Heap_Allocator *allocator, Free_Chunck *free_chunck) {
    void *next_addr = (void*)((char*)free_chunck + free_chunck->size);  

    if (free_chunck->next != NULL && next_addr == free_chunck->next) {
        free_chunck->size += free_chunck->next->size;
        free_list_remove_chunck(allocator, free_chunck->next);
    }

    if (free_chunck->previous != NULL) {
        void *addr_from_previous = (void*)((char*)free_chunck->previous + free_chunck->previous->size);
        if (addr_from_previous == free_chunck) {
            free_chunck->previous->size += free_chunck->size;
            free_list_remove_chunck(allocator, free_chunck);
        }
    }
}

// ==========================
//  FUNÇÕES PRINCIPAIS
// ==========================

Heap_Allocator* heap_create(void *memory_start, size_t memory_size) {
    // Verifica se há espaço suficiente para a struct de controle
    if (memory_size < sizeof(Heap_Allocator) + 128) {
        return NULL; // Erro: Memória insuficiente
    }

    // A Mágica: O Alocador vive no início da memória passada!
    Heap_Allocator *allocator = (Heap_Allocator*)memory_start;

    // Configura os ponteiros
    allocator->capacity = memory_size;
    allocator->used = sizeof(Heap_Allocator); // A própria struct ocupa espaço
    
    // Os dados gerenciáveis começam logo após a struct do alocador
    allocator->data = (char*)memory_start + sizeof(Heap_Allocator);
    
    // Inicializa o primeiro Free Chunk (o resto da memória)
    Free_Chunck *first_chunk = (Free_Chunck*)allocator->data;
    first_chunk->size = memory_size - sizeof(Heap_Allocator);
    first_chunk->next = NULL;
    first_chunk->previous = NULL;

    allocator->free_list = first_chunk;
    
    return allocator; // Retorna o ponteiro para o usuário
}

void *heap_alloc(Heap_Allocator *allocator, size_t size) {
    if (size < sizeof(Free_Chunck)) {
        size = sizeof(Free_Chunck);
    }

    Free_Chunck *chunck = find_best_chunck(allocator->free_list, size, DEFAULT_ALIGNMENT);
    
    if (chunck == NULL) return NULL;

    size_t padding = calculate_padding((uintptr_t)chunck, sizeof(Allocation_Header), DEFAULT_ALIGNMENT);

    size_t required_space = size + padding;
    size_t remaining_space = chunck->size - required_space;

    if (remaining_space > MIN_CHUNCK_SIZE) {
        uintptr_t chunck_addr = (uintptr_t)chunck;
        uintptr_t next_free_addr = chunck_addr + required_space;

        Free_Chunck *new_chunck = (Free_Chunck*)next_free_addr;
        new_chunck->size = remaining_space;
        free_list_insert_chunck(allocator, chunck->previous, new_chunck);
    }
    free_list_remove_chunck(allocator, chunck);

    size_t aligment_padding = padding - sizeof(Allocation_Header);
    Allocation_Header *header_ptr = (Allocation_Header*)((uintptr_t)chunck + aligment_padding);
    header_ptr->size = required_space;
    header_ptr->padding = aligment_padding;

    // DEBUG LIST
    debug_list.used_chuncks[debug_list.count] = header_ptr;
    debug_list.count++;

    allocator->used += required_space;

    void *requested_mem_ptr = (void*)((char*)header_ptr + sizeof(Allocation_Header));

    return requested_mem_ptr;
}

void heap_free(Heap_Allocator *allocator, void *ptr) {
    Allocation_Header *header_ptr = (Allocation_Header*)((char*)ptr - sizeof(Allocation_Header));

    debug_remove_chunck(header_ptr);

    Free_Chunck *freed_chunck = (Free_Chunck*)((char*)header_ptr - header_ptr->padding);
    freed_chunck->size = header_ptr->size;
    freed_chunck->next = NULL;

    Free_Chunck *curr = allocator->free_list;

    if (curr == NULL || (void*)freed_chunck < (void*)curr) {
        free_list_insert_chunck(allocator, NULL, freed_chunck);
    } else {
        while (curr->next != NULL && (void*)freed_chunck > (void*)curr->next) {
            curr = curr->next;
        }
        // Insere APÓS o curr
        free_list_insert_chunck(allocator, curr, freed_chunck);
    }

    allocator->used -= freed_chunck->size;

    free_list_coalescence_chunck(allocator, freed_chunck);
}
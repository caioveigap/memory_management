#pragma once

#include <stddef.h>


#define HEAP_CAPACITY (1024 * 1024 * 64) // 64 MiB
#define DEFAULT_ALIGNMENT (2 * sizeof(void*))
#define MIN_CHUNCK_SIZE 32

typedef struct Heap_Allocator Heap_Allocator;


Heap_Allocator* heap_create(void *memory_start, size_t memory_size);
void *heap_alloc(Heap_Allocator *allocator, size_t size);
void heap_free(Heap_Allocator *allocator, void *ptr);

void print_allocated_chunck_list();
void print_free_chunck_list(Heap_Allocator *allocator);
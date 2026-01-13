#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#define DEFAULT_ALIGNMENT (2*sizeof(void *))
#define BUFFER_LENGHT 8192

typedef struct Arena {
    unsigned char *buffer;
    size_t buffer_len;
    size_t prev_offset;
    size_t curr_offset;
} Arena;

void arena_init(Arena *arena, char *backing_buffer, size_t buf_len);
void *arena_alloc(Arena *arena, size_t size);
void *arena_resize(Arena *arena, void *old_memory_ptr, size_t old_memory_size, size_t new_size);
void arena_free_all(Arena *arena);
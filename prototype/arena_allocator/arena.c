#include "arena.h"

bool is_power_of_two(uintptr_t x) {
    return (x & (x-1)) == 0;
}

uintptr_t align_forward(uintptr_t ptr, size_t align) {
    uintptr_t p, a, modulo;

    assert(is_power_of_two(align));

    p = ptr;
    a = align;
    // Faster modulo operation. Translate to (p % a)
    modulo = p & (a-1);

    if (modulo != 0) {
        // If 'p' address is not aligned, push the address to the
		// next value which is aligned
        p += a - modulo;
    }
    return p;
}

void *arena_alloc_align(Arena *arena, size_t size, size_t align) {
    uintptr_t curr_ptr = (uintptr_t)arena->buffer + (uintptr_t)arena->curr_offset;
    uintptr_t offset = align_forward(curr_ptr, align);
    offset -= (uintptr_t)arena->buffer;

    if (offset + size <= arena->buffer_len) {
        void *ptr = &(arena->buffer[offset]);
        arena->prev_offset = offset;
        arena->curr_offset = offset + size;

        memset(ptr, 0, size);
        return ptr;
    }
    return NULL;
}

void *arena_alloc(Arena *arena, size_t size) {
    return arena_alloc_align(arena, size, DEFAULT_ALIGNMENT);
}

void *arena_resize_align(Arena *arena, void *old_memory_ptr, size_t old_memory_size, size_t new_size, size_t align) {
    assert(is_power_of_two(align));

    unsigned char *old_ptr = (unsigned char*)old_memory_ptr;

    if (old_ptr == NULL || old_memory_size == 0) {
        return arena_alloc_align(arena, new_size, align);
    }
    else if (arena->buffer <= old_ptr && old_ptr < arena->buffer + arena->buffer_len) {
        if (arena->buffer + arena->prev_offset == old_ptr) {
            arena->curr_offset = arena->prev_offset + new_size;
            if (new_size > old_memory_size) {
                memset(&arena->buffer[arena->curr_offset], 0, new_size - old_memory_size);
            }
            return old_memory_ptr;
        } else {
        void *new_memory = arena_alloc_align(arena, new_size, align);
        size_t copy_size = old_memory_size < new_size ? old_memory_size : new_size;
        memmove(new_memory, old_memory_ptr, copy_size);
        return new_memory;
        } 
    } else {
        perror("Memory out of arena bounds");
        return NULL;
    }
}

void *arena_resize(Arena *arena, void *old_memory_ptr, size_t old_memory_size, size_t new_size) {
    return arena_resize_align(arena, old_memory_ptr, old_memory_size, new_size, DEFAULT_ALIGNMENT);
}

void arena_init(Arena *arena, char *backing_buffer, size_t buf_len) {
    arena->buffer = backing_buffer;
    arena->buffer_len = buf_len;
    arena->prev_offset = 0;
    arena->curr_offset = 0;
}

void arena_free_all(Arena *arena) {
    arena->curr_offset = 0;
    arena->prev_offset = 0;
}

void print_arena_state(Arena *a) {
    uintptr_t buf_curr_addr = (uintptr_t)a->buffer + (uintptr_t)a->curr_offset;
    printf("\n===Arena State===\n");
    printf("Buffer Start Address: %ld\n", (uintptr_t)a->buffer);
    printf("Buffer Pointer Address: %ld\n", buf_curr_addr);
    printf("Previous offset: %ld\n", a->prev_offset);
    printf("Current offset: %ld\n", a->curr_offset);
    printf("Buffer Lenght: %ld\n", a->buffer_len);
}
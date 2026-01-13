#include "linear_allocator.h"


int main() {
    char backing_buffer[8192];
    Arena default_arena;
    Arena *arena_ptr = &default_arena;

    arena_init(arena_ptr, backing_buffer, 8192);

    char *string = arena_alloc(arena_ptr, (32 * sizeof(char)));
    char *cpy_string = "COPY STRING TEST!";

    strcpy(string, cpy_string);
    printf("%s", string);   
}
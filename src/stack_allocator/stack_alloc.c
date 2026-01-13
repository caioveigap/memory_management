#include "stack_allocator.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

bool is_power_of_two(uintptr_t x) {
    return (x & (x-1)) == 0;
}

size_t padding_calculation(uintptr_t curr_address, size_t alignment, size_t header_size) {
    uintptr_t modulo, padding;

    padding = 0;

    modulo = curr_address % alignment;
    if (modulo != 0) {
        padding += alignment - modulo;
    }

    printf("Modulo: %ld\n", modulo);

    uintptr_t needed_space = (uintptr_t)header_size;
    
    if (padding < needed_space) {
        needed_space -= padding;
        if ((needed_space & (alignment-1)) != 0) {
            padding += alignment * (1 + (needed_space/alignment));
        } else {
            padding += alignment * (needed_space / alignment);
        }
    }


    return (size_t)padding;
}

void stack_init(Stack *stack, void *backing_buffer, size_t backing_buffer_len) {
    stack->buffer = (unsigned char *)backing_buffer;
    stack->capacity = backing_buffer_len;
    stack->offset = 0;
}

void *stack_alloc_align(Stack *stack, size_t size, size_t alignment) {
    uintptr_t curr_addr, aligned_addr, header_addr;
    size_t padding, header_size;
    Stack_Header *header;

    header_size = sizeof(Stack_Header);

    assert(is_power_of_two(alignment));

    curr_addr = (uintptr_t)stack->buffer + (uintptr_t)stack->offset;
    padding = padding_calculation(curr_addr, alignment, sizeof(Stack_Header));

    if (stack->offset + padding + size > stack->capacity) {
        return NULL; //OUT OF MEMORY
    }
    
    aligned_addr = curr_addr + padding;
    header_addr = aligned_addr - header_size;

    header = (Stack_Header*)header_addr;
    header->padding = (uint8_t)padding;

    stack->offset += padding + size;
    memset((void*)aligned_addr, 0, size);

    return (void*)aligned_addr;
''
}

void *stack_alloc(Stack *stack, size_t size) {
    return stack_alloc_align(stack, size, DEFAULT_ALIGNMENT);
}

void stack_free(Stack *stack, void *ptr) {
    if (ptr != NULL) {
        uintptr_t start, end, curr_adr, header_addr;
        Stack_Header *header;
        size_t prev_offset;

        start = (uintptr_t)stack->buffer;
        end = start + (uintptr_t)stack->capacity;
        curr_adr = (uintptr_t)ptr;

        if (curr_adr < start || curr_adr > end) {
            perror("Out of bounds memory address passed to the allocator (free)");       
        }
        if (curr_adr >= start + (uintptr_t)stack->offset) return;

        header_addr = curr_adr - sizeof(Stack_Header);
        header = (Stack_Header*)header_addr;

        prev_offset = (size_t)(curr_adr - (uintptr_t)header->padding - start);
        stack->offset = prev_offset;
    }
}

void stack_free_all(Stack *stack) {
    stack->offset = 0;
}

void *stack_resize_align(Stack *stack, void *ptr, size_t old_size, size_t new_size, size_t alignment) {
    assert(is_power_of_two(alignment));

    if (ptr == NULL || new_size == 0) {
        return stack_alloc_align(stack, new_size, alignment);
    }

    uintptr_t mem_ptr = (uintptr_t)ptr;
    uintptr_t start = (uintptr_t)stack->buffer;
    uintptr_t end = start + (uintptr_t)stack->capacity;
    
    if (mem_ptr < start || mem_ptr > end) {
        perror("Out of bounds memory address passed to stack allocator(resize)");
    }
    if (mem_ptr >= start + (uintptr_t)stack->offset) {
        return NULL;
    }
    if (old_size == new_size) {
        return ptr;
    }

    void *new_addr = stack_alloc_align(stack, new_size, alignment);
    memmove(new_addr, ptr, old_size);

    return new_addr;
}

void *stack_resize(Stack *stack, void *ptr, size_t old_size, size_t new_size) {
    return stack_resize_align(stack, ptr, old_size, new_size, DEFAULT_ALIGNMENT);
}

void print_stack_status(Stack *stack) {
    printf("\n=== STACK STATUS ===\n");
    printf("Capacity: %zu bytes\n", stack->capacity);
    printf("Current Offset: %zu bytes\n", stack->offset);
    printf("Buffer Start Address: %p\n", stack->buffer);
    printf("----------------------------------------------------------------\n");
    printf("Offset  | Bytes (Hex)                                     | ASCII\n");
    printf("----------------------------------------------------------------\n");

    if (stack->offset == 0) {
        printf("   (Stack Vazia)\n");
        printf("----------------------------------------------------------------\n");
        return;
    }

    // Itera pelo buffer em blocos de 16 bytes (padrão de visualização)
    for (size_t i = 0; i < stack->offset; i += 16) {
        
        // 1. Imprime o Offset atual (ex: 0000, 0010, 0020)
        printf("%04zX    | ", i);

        // 2. Imprime os bytes em Hexadecimal
        for (size_t j = 0; j < 16; j++) {
            if (i + j < stack->offset) {
                printf("%02X ", stack->buffer[i + j]);
            } else {
                printf("   "); // Espaçamento se acabar antes de 16 bytes
            }
        }

        printf("| ");

        // 3. Imprime a representação ASCII
        for (size_t j = 0; j < 16; j++) {
            if (i + j < stack->offset) {
                unsigned char c = stack->buffer[i + j];
                // isprint verifica se é letra/número/símbolo visível
                printf("%c", isprint(c) ? c : '.');
            }
        }
        
        printf("\n");
    }
    printf("----------------------------------------------------------------\n");
}
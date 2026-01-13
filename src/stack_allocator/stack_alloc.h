#pragma once
#include <stdio.h>
#include <stdint.h>

#define DEFAULT_ALIGNMENT (2*sizeof(void *))

typedef struct Stack {
    unsigned char *buffer;
    size_t capacity;
    size_t offset;  
} Stack;

typedef struct Stack_Header {
    uint8_t padding;
} Stack_Header;


void stack_init(Stack *stack, void *backing_buffer, size_t backing_buffer_len);
void *stack_alloc(Stack *stack, size_t size);
void *stack_alloc_align(Stack *stack, size_t size, size_t alignment);
void stack_free(Stack *stack, void *ptr);
void stack_free_all(Stack *stack);
void *stack_resize_align(Stack *stack, void *ptr, size_t old_size, size_t new_size, size_t alignment);
void *stack_resize(Stack *stack, void *ptr, size_t old_size, size_t new_size);
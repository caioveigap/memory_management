#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h> // Faltava incluir stdbool.h para o tipo 'bool'

/* ==============================
FUNÇÕES UTILITÁRIAS + DEBUG
==============================
*/

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

// Adicionado 'static inline' em todas as funções abaixo:

static inline void print_addr(void *ptr, char *label) {
    printf("%s Address: %ld\n", label, (uintptr_t)ptr);
}

static inline bool is_power_of_two(size_t x) {
    return (x & (x-1)) == 0;
}

static inline size_t align_size(size_t size, size_t alignment) {
    size_t aligned_size = size; // Simplificação da lógica inicial
    if (aligned_size < sizeof(void*)) aligned_size = sizeof(void*);

    size_t remainder = aligned_size % alignment;
    if (remainder != 0) {
        aligned_size += alignment - remainder;
    }
    return aligned_size;
}

static inline void *align_ptr(void *ptr, size_t alignment) {
    uintptr_t addr = (uintptr_t)ptr;
    size_t remainder = addr % alignment;
    if (remainder == 0) return ptr;
    return (void*)(addr + (alignment - remainder));
}

static inline u32 fast_log2(u32 n) {
    if (n == 0) return 0; 
    return (sizeof(unsigned int) * 8) - 1 - __builtin_clz(n);
}

static inline u32 round_up_pow2(u32 n) {
    if (n <= 1) return 1; 
    return 1 << (32 - __builtin_clz(n - 1));
}
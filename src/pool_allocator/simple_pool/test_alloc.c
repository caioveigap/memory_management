#include "pool_alloc.h"
#include <stdio.h>
#include <string.h>

typedef struct Array {
    char buf[64];
} Array;

int main() {
    Pool pool_allocator;

    pool_init(&pool_allocator, 32);
    
    Chunck *base_array = pool_allocator.chunck_array;

    char *buf_1 = pool_alloc(&pool_allocator);
    char *buf_2 = pool_alloc(&pool_allocator);
    char *buf_3 = pool_alloc(&pool_allocator);
    char *buf_4 = pool_alloc(&pool_allocator);
    char *buf_5 = pool_alloc(&pool_allocator);
    char *buf_6 = pool_alloc(&pool_allocator);
    char *buf_7 = pool_alloc(&pool_allocator);
    char *buf_8 = pool_alloc(&pool_allocator);
    
    memset(buf_1, 'A', sizeof(Chunck));
    memset(buf_2, 'B', sizeof(Chunck));
    memset(buf_3, 'C', sizeof(Chunck));
    memset(buf_4, 'D', sizeof(Chunck));
    memset(buf_5, 'E', sizeof(Chunck));
    memset(buf_6, 'F', sizeof(Chunck));
    memset(buf_7, 'G', sizeof(Chunck));
    memset(buf_8, 'H', sizeof(Chunck));

    buf_1[63] = '\0';
    buf_2[63] = '\0';
    buf_3[63] = '\0';
    buf_4[63] = '\0';
    buf_5[63] = '\0';
    buf_6[63] = '\0';
    buf_7[63] = '\0';
    buf_8[63] = '\0';

    for (size_t i = 0; i < pool_allocator.chunck_count; i++) {
        printf("%s", base_array[i].buffer);
        printf("\n");
    }

}
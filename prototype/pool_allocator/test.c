#include "pool.h"
#include <stdio.h>
#include <string.h>

typedef struct Block_Data {
    char data[80];
} Block_Data;


int main() {
    Allocator *alloc = allocator_create();

    size_t block_size = 80;
    size_t num_of_blocks = 32;
    Pool *p = pool_create(block_size, num_of_blocks, DEFAULT_ALIGNMENT);

    Block_Data *data_01 = pool_alloc(p);
    Block_Data *data_02 = pool_alloc(p);
    Block_Data *data_03 = pool_alloc(p);

    printf("Endereço de memoria do bloco 1: %p\n\n", data_01);
    printf("Endereço de memoria do bloco 2: %p\n\n", data_02);
    printf("Endereço de memoria do bloco 3: %p\n\n", data_03);


    memset(data_01, 0xAA, sizeof(Block_Data));
    memset(data_02, 0xAA, sizeof(Block_Data));
    memset(data_03, 0xAA, sizeof(Block_Data));

    pool_free(data_02);

    Block_Data *data_04 = pool_alloc(p);
    printf("Endereço de memoria do bloco 4: %p\n\n", data_04);



    pool_destroy(p);
}
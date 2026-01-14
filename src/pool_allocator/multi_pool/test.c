#include "multipool_alloc.h"
#include <stdio.h>

#define BLOCK_SIZE 256
#define NUMS_OF_BLOCKS 256

struct Item {
    int type;
    size_t quantity;
};

struct Entity {
    char name[32]; //debug
    int type;
    double x, y, z;
    double velocity_x, velocity_y;

    size_t remaining_ammo;
    struct Item items[8];
};

int main() {
    Allocator *my_alloc = allocator_create();

    Pool *my_pool = pool_create(my_alloc, BLOCK_SIZE, NUMS_OF_BLOCKS, DEFAULT_ALIGNMENT);

    printf("Entity size: %ld", sizeof(struct Entity));
}
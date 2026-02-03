#include "backend_manager.h"
#include <stdio.h>


int main() {
    backend_init(RESERVED_MEMORY_REGION_SIZE);
    int *array1 = backend_alloc((1000 * sizeof(int)), OWNER_DEBUG);

    for (int i = 0; i < PAGE_SIZE; i++) {
        array1[i] = 255;
    }

    debug_print_bins();
    
    printf("\n AFTER FREE\n");

    backend_free(array1);
    
    debug_print_bins();
}
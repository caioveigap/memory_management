#include "../include/backend_manager.h"
#include "../include/utilities.h"


#define MIN_HEAP_BLOCK_SIZE 32
#define MAX_BINS 12

typedef struct Allocation_Header {
    size_t size;
    size_t prev_size;
    u8 is_free;
} Allocation_Header __attribute__((aligned(16)));



typedef struct Free_Block {
    Allocation_Header header;
    struct Free_Block *next;
    struct Free_Block *prev;
} Free_Block;

// typedef struct Heap_Allocator {

// } Heap_Allocator;
#pragma once

#include <stddef.h>
#include <unistd.h>
#include <stdint.h>

typedef struct Free_List_Node Free_List_Node;
typedef struct Allocation_Header Allocation_Header;


struct Free_List_Node {
    Free_List_Node *next;
    size_t size;
};

struct Allocation_Header {
    size_t size;
    size_t padding;
}
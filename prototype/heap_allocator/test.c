#include "heap.h"

char heap[HEAP_CAPACITY];

int main() {
    Heap_Allocator *my_allocator = heap_create(heap, HEAP_CAPACITY);

    int *arr1 = heap_alloc(my_allocator, (20 * sizeof(int)));
    int *arr2 = heap_alloc(my_allocator, (16 * sizeof(int)));
    int *arr3 = heap_alloc(my_allocator, (30 * sizeof(int)));
    int *arr4 = heap_alloc(my_allocator, (40 * sizeof(int)));
    int *arr5 = heap_alloc(my_allocator, (120 * sizeof(int)));
    int *arr6 = heap_alloc(my_allocator, (46 * sizeof(int)));
    int *arr7 = heap_alloc(my_allocator, (512 * sizeof(int)));



    heap_free(my_allocator, arr2);
    heap_free(my_allocator, arr6);

    print_allocated_chunck_list();
    print_free_chunck_list(my_allocator);
}
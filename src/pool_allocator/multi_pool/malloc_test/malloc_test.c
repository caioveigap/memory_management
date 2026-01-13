#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main() {
    unsigned char *mem = (unsigned char*)malloc(100);
    unsigned char *mem_temp = mem;

    for (size_t i = 0; i < 26; i++) {
        mem[i] = 'A' + i;
        mem_temp++;
    }

    memset(mem_temp, 0xAA, (100 - 26));

    for (size_t i = 0; i < 100; i++) {
        printf("%c ", mem[i]);
    }
}
#include <stddef.h>

// issue with the code is that if we try to free it, we don't know the size of the block
// -> we need to store the size somewhere
void *malloc(size_t size) {
    void* block;
    block = sbrk(size);
    if (block == (void*) - 1) return NULL;
    return block;
}
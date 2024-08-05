#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

typedef char ALIGN[16]; // align header to be 16 bytes

union header{
    struct {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;

    ALIGN stub;
};
typedef union header header_t;

header_t *head = NULL, *tail = NULL; // keep track of the list

pthread_mutex_t global_malloc_lock; // prevent concurrently accessing the same memory

// traverse LL and serach for a block of memory that is free and can large enough
header_t *get_free_block(size_t size) {
    header_t *curr = head;
    while(curr) {
        if (curr -> s.is_free && curr -> s.size >= size) {
            return curr;
        }
        curr = curr -> s.next;
    }
    return NULL;
}

void free(void *block) {
    header_t *header, *tmp;
    void *programbreak;

    if (!block) return;

    pthread_mutex_lock(&global_malloc_lock);
    header = (header_t*) block - 1; // get a pointer that is behind the block in the size of a header (done by casting!)

    programbreak = sbrk(0); // check where the heap ends

    // find the end of the current block and compare with the programbreak to determine if we're at the end or not
    // if it's, we can shrink the heap and release memory to OS
    if ((char*) block + header -> s.size == programbreak) {
        if (head == tail) head = tail = NULL; // reflect the loss of the last block
        else {
            tmp = head;
            while (tmp) {
                if (tmp -> s.next == tail) {
                    tmp -> s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp -> s.next;
            }
        }
        // decrement heap size
        sbrk(0 - sizeof(header_t) - header -> s.size);
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }

    header -> s.is_free = 1;
    pthread_mutex_unlock(&global_malloc_lock);
}

void *malloc(size_t size) {
    size_t total_size;
    void *block;
    header_t *header;

    // invalid size, 0
    if (!size) return NULL;
    pthread_mutex_lock(&global_malloc_lock);

    header = get_free_block(size);
    if (header) {
        header -> s.is_free = 0; // mark it as not free
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL; // return a pointer to the byte after the end of the header : memory block
    }

    // if no big enough free block -> extend the heap
    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);
    if (block == (void *) - 1) {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }

    // update the header with new size
    header = block;
    header -> s.size = size;
    header -> s.is_free = 0;
    header -> s.next = NULL;
    
    if (!head) head = header;
    if (tail) tail -> s.next = header;
    tail = header;
    pthread_mutex_unlock(&global_malloc_lock);
    return (void *) (header + 1);
}

// calloc : allocates memory for an array of num elements of nsize bytes each and returns a ptr to the memory
// memory is set to 0
void *calloc(size_t num, size_t nsize) {
    size_t size;
    void *block;

    if (!num || !size) return NULL;

    size = num * nsize; // total size to allocate
    if (nsize != size / num) return NULL; // mul overflow
    block = malloc(size); 
    if (!block) return NULL;

    memset(block, 0, size); // memset : set size bytes of block to 0 -> clears the allocated memory to zeroes
    return block;
}

// change the size of the given memory to the given size
void* realloc(void *block, size_t size) {
    header_t *header;

    void *ret;
    if (!block || !size) return malloc(size);

    header = (header_t*) block-1; // remove by 1 header size
    if (header -> s.size >= size) return block; // if we already have enough space, no need to create more

    // allocate memory for the given size, copy the stuff from block to ret in s.size bytes and free the old block
    ret = malloc(size); 
    if (ret) {
        memcpy(ret, block, header->s.size);
        free(block);
    }
    return ret;
}
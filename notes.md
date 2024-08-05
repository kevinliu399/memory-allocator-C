this is an implementation of memory allocation by following this [tutorial](https://arjunsreedharan.org/post/148675821737/memory-allocators-101-write-a-simple-memory)

### Notes
memory layout of a program: A process runs within its own virtual address space, distinct from the others
1. Text: Binary Instruction to be executed by the CPU
2. Data: Non-initalized static data
3. BSS (Block Started by Symbol): Zero-initialized static data
4. Heap: Dynamically allocated memory
5. Stack: Automatic vars, function args, and copy of base ptrs etc

N.B. The stack and the heap grow in the opposite direction
Also, "data segment" > bss + data + heap and is delimited by a pointer named "brk" (end of heap)

To allocate memory in heap -> request sys to increment brk pointer and vice versa
```c
sbrk(0)  // current address of brk
sbrk(x)  // increment brk by x bytes
sbrk(-x) // decrement brk by x bytes

// returns (void *)-1 on failure
```

But sbrk is not thread-safe as it uses in LIFO order

```C
#include <stddef.h>

// issue with the code is that if we try to free it, we don't know the size of the block
// -> we need to store the size somewhere
void *malloc(size_t size) {
    void* block;
    block = sbrk(size);
    if (block == (void*) - 1) return NULL;
    return block;
}
```

heap memory is contiguous (def. touching or connected in an unbroken series) so we can't free a block in the middle of the heap 
-> make a distinction between freeing memory and releasing it when it's not at the end of the heap

what this means is that we only need to mark it for the moment if we're trying to free it, we can deal with it later
in the future if we try to call malloc again

```C
// simple implementation to keep track of allocated memory
struct header_t {
    size_t size;
    unsigned is_free;
    union header_t *next; // LL to traverse
}
```

the idea is to do `sbrk(total_size)` where total_size = size + sizeof(header_t) to keep track of the size of the block

mutex: a mutex is a lock that we set before using a shared resource and release after using it
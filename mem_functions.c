#include "mem.h"
extern BLOCK_HEADER* first_header;


int Is_Free(BLOCK_HEADER* header) {
    int size = 0;
    size = header->size_alloc;
    if (size % 16 == 0) {
        return 1;
    }
    return 0;
}

// return a pointer to the payload
// if a large enough free block isn't available, return NULL
void* Mem_Alloc(int size) {
    // find a free block that's big enough
    BLOCK_HEADER *header = first_header;
    
    while (1) {
        if (Is_Free(header) && header->payload >= size) {
            // we found a free header whose payload has enough space
            break;
        }

        // this is the last header. no space available, return null
        if (header->size_alloc == 1) {
            return NULL;
        } 

        // keep moving until we find a free block that is big enough
        if (Is_Free(header)) {
            header = (BLOCK_HEADER *)((unsigned long)header + (header->payload) + 8);
        } else {
            header = (BLOCK_HEADER *)((unsigned long)header + (header->size_alloc) - 1);
        }
    }
    
    // allocating memory

    int block_size = header->payload;
    
    // update the header's payload with size requested by user
    header->payload = size;
    
    // compute padding - need to decide here whether to split block or not
    int padding;
    int split = 1;
    if (block_size - size >= 16) {
        // (if padding size is greater than or equal to 16 split the block)
        if ((8 + size) % 16 == 0) {
            padding = 0;
        } else {
            padding = 16 - (8 + size) % 16;
        }
    } else {
        split = 0;
        padding = block_size - size;
    }
    
    // update header's size_alloc and add one since this block is now allocated
    header->size_alloc = 8 + size + padding + 1;

    // store pointer to be returned to the user
    void *USER_POINTER = (void *)((unsigned long)header + 8);
    
    // split if necessary (if padding size is greater than or equal to 16 split the block)
    if (split) {
        // create new header
        BLOCK_HEADER *new_header = (BLOCK_HEADER *)((unsigned long)header + 8 + size + padding);
        new_header->size_alloc = block_size - (size + padding);  
        new_header->payload = block_size - (8 + size + padding);    
    }
    return USER_POINTER;
}


// return 0 on success
// return -1 if the input ptr was invalid
int Mem_Free(void *ptr){
    // traverse the list and check all pointers to find the correct block 
    // if you reach the end of the list without finding it return -1
    BLOCK_HEADER *header = first_header;
    int header_count = 0;
    while (1) {
        if (!Is_Free(header) && (void *)((unsigned long)header + 8) == ptr) {
            // found the relevant header 
            break;
        }
        // reached the end of list (this is the last header). return -1.
        if (header->size_alloc == 1) {
            return -1;
        } 

        // keep moving until we find the relevant header
        if (Is_Free(header)) {
            header = (BLOCK_HEADER *)((unsigned long)header + (header->payload) + 8);
        } else {
            header = (BLOCK_HEADER *)((unsigned long)header + (header->size_alloc) - 1);
        }
        header_count++;
    }

    // free the block - note we are at the header and not the pointer
    // just change the allocted bit to zero 
    // and update payload to represent the whole free block instead of just the space requested by the user

    int block_size = header->size_alloc - 1; 
    //printf("%d\n", block_size);
    header->size_alloc = block_size;
    header->payload = header->size_alloc - 8;

    // coalesce adjacent free blocks
    // TODO

    // checking the next header
    BLOCK_HEADER *next_header = (BLOCK_HEADER *)((unsigned long)header + (header->payload) + 8);

    // if free then merge by updating current header's block size
    if (Is_Free(next_header)) {
        header->size_alloc = header->size_alloc + next_header->size_alloc;
        header->payload = header->size_alloc - 8;
    }
    //printf("%d\n", header_count);
    // checking the previous header
    BLOCK_HEADER *prev_header = first_header;
    for (int i = 0; i < header_count - 1; i++) {
        if (Is_Free(prev_header)) {
            prev_header = (BLOCK_HEADER *)((unsigned long)prev_header + (prev_header->payload) + 8);
        } else {
            prev_header = (BLOCK_HEADER *)((unsigned long)prev_header + (prev_header->size_alloc) - 1);
        }
    }

    //printf("%p\n", prev_header);
    if (header_count > 0 && Is_Free(prev_header)) {
        prev_header->size_alloc = prev_header->size_alloc + header->size_alloc;
        prev_header->payload = prev_header->size_alloc - 8;
    }

    return 0;
}


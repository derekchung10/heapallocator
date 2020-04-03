#include "allocator.h"
#include "debug_break.h"
#include "math.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

static void *start;
static size_t size = 0;
static void *last;
static void *prev;
static size_t prev_size = 0;

size_t roundup(size_t round) {
    return (ALIGNMENT - round) % ALIGNMENT + round;
}

bool myinit(void *heap_start, size_t heap_size) {  
    if (heap_size + (size_t)heap_start < heap_size) {
        return false;
    }
    if (heap_size < (size_t)ALIGNMENT + 8L) {
        return false;
    }
    if (size !=  0) {
        size_t count = size / 8;
        for (int i = 0; i < count; i ++) {
            *((char *)(start) + i) = (char)0;
        }        
    }
    size = heap_size;
    start = heap_start;
    last = heap_start;
    prev = heap_start;
    prev_size = heap_size - 8;
    *(size_t *)start = prev_size;
    return true;
}

void *mymalloc(size_t requested_size) {
    if (requested_size > MAX_REQUEST_SIZE) {
        return NULL;
    }
    if (requested_size == (size_t)0) {
        return NULL;
    }
    void *current = start;
    size_t round = roundup(requested_size);
    if (prev_size <= round) {
        current = prev;
    }
    while (true) {
        size_t header = *(size_t *)current - (size_t)1;
        if (header << 63) {
            header += 1L;    
            if (header >= round) {
                prev = current;
                prev_size = round;
                if (current == last) {
                    last = (char *)current + round + 8;
                    *(size_t *)last = header - round - 8;
                    *(size_t *)current = round;
                } else if (header - round >= (size_t)ALIGNMENT) {
                    prev = (char *)current + round + 8;
                    *(size_t *)prev = header - round - (size_t)8;
                    *(size_t *)current = round;
                    prev = current;
                }
                *(size_t*)current += 1L;
                void *ret = (char *)current + 8;
                return ret;
            }
        }     
        if (current == last) {
            break;
        }
        current = (char *)current + header + 8;
    }
    return NULL;
}

void myfree(void *ptr) {
    if (ptr != NULL) {
        void *edit = (char *)ptr - 8;
        size_t newSize = *(size_t *)edit;
        if (newSize << 63) {
            *(size_t *)(edit) -= 1L;
            prev_size = newSize - 1L;
            prev = edit;
        }
    }
}

void *myrealloc(void *old_ptr, size_t new_size) {
    if (old_ptr == NULL) {
        return mymalloc(new_size);
    }
    if (new_size == 0) {
        myfree(old_ptr);
        return old_ptr;
    }
    void *ptr;
    if (roundup(*((size_t *)(old_ptr) - 1)) - (size_t)1 >= roundup(new_size)) {
        return old_ptr;
    }
    if ((ptr = mymalloc(new_size)) != NULL) {
        memcpy(ptr, old_ptr, new_size);
        myfree(old_ptr);
        return ptr;
    }
    return NULL;
}

bool validate_heap() {
    /**
    char **arr = mymalloc(sizeof(char *) * 4);
    char *word = "HELLO";
    arr[0] = word;
    arr = myrealloc(sizeof(char *) * 2);
    if (arr[0] != "HELLO") {
        return false;
    }
    myfree(arr);
    */
    return true;
}

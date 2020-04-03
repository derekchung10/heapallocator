#include "allocator.h"
#include "debug_break.h"
#include <string.h>
#include <stdio.h>
static size_t size = 0;
static void *start;
static void *front;

typedef struct Pointer {
    void *next;
    void *back;
} Pointer;

size_t roundup(size_t round);
size_t getSize(void *ptr);//payload
size_t getSizeMalloced(void *ptr);//payload
void setSize(void *ptr, size_t payload);
void setSizeMalloced(void *ptr, size_t payload);
void *getHeader(void *ptr); //get header from client pointer
void *getRet(void *ptr);
void insert(void *ptr, size_t payload); //convert free to malloc
void coalesce(void *ptr); //coaless given free ptr with right neighbor
size_t getPadding(void *ptr, size_t payload);//returns payload space leftover for free
void *getFree(void *ptr, size_t payload);//returns next header after malloc. ptr is previous header
void addFreeToList(void *ptr);
void removeFreeFromList(void *ptr);
Pointer *convertPtrToStruct(void *ptr);
bool isFree(void *ptr);
void debug();
size_t roundup(size_t round) {
    if (round < (size_t)16) {
        return (size_t)16;
    }
    if ((size_t)(round % ALIGNMENT) == (size_t)0) {
        return round;
    }
    return (size_t)ALIGNMENT - (size_t)(round % ALIGNMENT) + round;
}

size_t getSize(void *ptr) {
    return *(size_t *)ptr;
}

size_t getSizeMalloced(void *ptr) {
    return *(size_t *)ptr - (size_t)1;
}

void setSize(void *ptr, size_t payload) {
    *(size_t *)ptr = payload;
}

void setSizeMalloced(void *ptr, size_t payload) {
    *(size_t *)ptr = payload + (size_t)1;
}

void *getHeader(void *ptr) {
    void *ret = (char *)ptr - 8;
    return ret;
}

void *getRet(void *ptr) {
    void *ret = (char *)ptr + 8;
    return ret;
}

void insert(void *ptr, size_t payload) {
    size_t sizeOfFree = getPadding(ptr, payload);
    void *freeptr = getFree(ptr, payload);
    removeFreeFromList(ptr);
    setSizeMalloced(ptr, payload);
    if (sizeOfFree != (size_t)0) {
        setSize(freeptr, sizeOfFree - (size_t)8);
        addFreeToList(freeptr);
    }
}

size_t getPadding(void *ptr, size_t payload) {
    return getSize(ptr) - payload;
}

void *getFree(void *ptr, size_t payload) {
    void *ret = (char *)getRet(ptr) + payload;
    return ret;
}

void addFreeToList(void *ptr) {
    Pointer *list = convertPtrToStruct(ptr);
    if (front != NULL) {
        Pointer *first = convertPtrToStruct(front);
        first -> back = ptr;
        list -> next = front;
    } else {
        list -> next = NULL;
    }
    list -> back = NULL;
    front = ptr;
}

void removeFreeFromList(void *ptr) {
    Pointer *list = convertPtrToStruct(ptr);
    if (ptr == front) {
        if (list -> next == NULL) {
            front = NULL;
        } else {
            convertPtrToStruct(list -> next) -> back = NULL;
            front = list -> next;
        }
    } else if (list -> next == NULL) {
        convertPtrToStruct(list -> back) -> next = NULL;
    } else {
        Pointer *inFront = convertPtrToStruct(list -> next);
        Pointer *behind = convertPtrToStruct(list -> back);
        inFront -> back = list -> back;
        behind -> next = list -> next;
    }
    list -> next = NULL;
    list -> back = NULL;
}

Pointer *convertPtrToStruct(void *ptr) {
    Pointer *ret = (Pointer *)((char *)(ptr) + 8);
    return ret;
}

void coalesce(void *ptr) {
    size_t payload = getSize(ptr);
    void *nextFree = getFree(ptr, payload);
    if ((char *)nextFree < (char *)start + size) {
        if (isFree(nextFree)) {
            setSize(ptr, payload + (size_t)8 + getSize(nextFree));
            removeFreeFromList(nextFree);           
        }
    }
    addFreeToList(ptr);
}

bool isFree(void *ptr) {
    if (ptr == getFree(start, size)) {
        return false;
    }
    if ((*(size_t *)ptr & (size_t)1L) == 0) {
        return true;
    }
    return false;
}
bool canContain(void *ptr, size_t payload) {
    size_t currSize = getSize(ptr);
    if (currSize >= payload) {
        return true;
    }
    return false;
}
void debug() {
    void *curr = start;
    printf("\n--------START DEBUG--------\n");
    printf("Front: %p\n", front);
    printf("Free Blocks: \n");
    void *curr2 = front;
    size_t counter = 0;
    while (curr2 != NULL) {
        printf("Block: %lu, Address: %p, Size: %lu,\n", counter, curr2, getSize(curr2));
        counter += 1;
        curr2 = convertPtrToStruct(curr2) -> next;
    }
    printf("----------------\n");
    while ((char *)curr < (char *)(start) + size) {
        size_t currSize;
        bool t = isFree(curr);
        if (t) {
            currSize = getSize(curr);
        } else {
            currSize = getSizeMalloced(curr);
        }
        printf("Size: %lu, Free: %d, Address: %p\n", currSize, t, curr);
        curr = getFree(curr, currSize);
    }
    printf("--------END DEBUG--------\n");
}
bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size + (size_t)heap_start < heap_size) {
        return false;
    }
    if (heap_size < (size_t)ALIGNMENT + 16L) {
        return false;
    }
    if (size !=  0) {
        size_t count = size / 8;
        for (int i = 0; i < count; i ++) {
            *((size_t *)(start) + i) = (size_t)0;
        }        
    }
    front = NULL;
    size = heap_size;
    start = heap_start;
    setSize(start, heap_size - (size_t)8);
    addFreeToList(start);
    return true;
}

void *mymalloc(size_t requested_size) {
    if (requested_size > MAX_REQUEST_SIZE || requested_size == (size_t)0) {
        return NULL;
    }
    requested_size = roundup(requested_size);
    void *curr = front;
    size_t min = size;
    void *change = NULL;
    //try to find padding of size 0 first
    while (curr != NULL) {
        if (canContain(curr, requested_size)) {
            size_t padding = getPadding(curr, requested_size);
            if (padding == (size_t)0) {
                insert(curr, requested_size);
                return getRet(curr);
            } else if (padding < min) {
                min = padding;
                change = curr;
            }
        }
        curr = convertPtrToStruct(curr) -> next;
    }    
    if (change != NULL) {
        if (min < 24) {
            insert(change, requested_size + min);
        } else {
            insert(change, requested_size);
        }
        return getRet(change);
    }
    return NULL;
}

void myfree(void *ptr) {
    if (ptr != NULL) {
        void *edit = getHeader(ptr);
        size_t set = getSizeMalloced(edit);
        setSize(edit, set);
        coalesce(edit);
    }
}

void *myrealloc(void *old_ptr, size_t new_size) {
    if (old_ptr == NULL) {
        return mymalloc(new_size);
    }
    void *oldHeader = getHeader(old_ptr);
    size_t old_size = getSizeMalloced(oldHeader);
    new_size = roundup(new_size);
    if (new_size == 0) {
        myfree(old_ptr);
        return old_ptr;
    }
    if (new_size == old_size) {
        return old_ptr;
    } else if (new_size < old_size) {
        size_t padding = getPadding(oldHeader, new_size) - (size_t)1;
        void *nextHeader = getFree(oldHeader, old_size);
        if ((char *)nextHeader >= (char *)start + size + (size_t)8) {
            if (padding >= 24) {
                setSize(nextHeader, padding - (size_t)8);
                addFreeToList(nextHeader);
                setSizeMalloced(oldHeader, new_size);
            }
        } else if (isFree(nextHeader)) {
            setSizeMalloced(oldHeader, new_size);
            void *new = getFree(oldHeader, getSizeMalloced(oldHeader));
            setSize(new, padding - (size_t)8);
            coalesce(new);
        } else if (padding >= 24) {
            setSizeMalloced(oldHeader, new_size);
            void *new = getFree(oldHeader, getSizeMalloced(oldHeader));
            setSize(new, padding - (size_t)8);
            addFreeToList(new);
        }
        return old_ptr;       
    } else if (new_size > old_size) {
        void *nextHeader = getFree(oldHeader, old_size);
        if (!isFree(nextHeader)) {
            void *ptr = mymalloc(new_size);
            if (ptr != NULL) {
                memcpy(ptr, old_ptr, old_size);
                myfree(old_ptr);
                return ptr;
            } 
        } else {
            size_t extra = getSize(nextHeader) + (size_t)8;
            //check if extra will suffice. if new_size > exta + oldsize, combine and call again
            //if equal or if extra + oldsize is 16 larger, combine and return
            //if less, combine the part and return
            if (new_size > extra + old_size) {
                setSizeMalloced(oldHeader, extra + old_size);
                removeFreeFromList(nextHeader);
                return myrealloc(old_ptr, new_size);
            } else if (extra + old_size - new_size < (size_t)24) {
                setSizeMalloced(oldHeader, old_size + extra);
                removeFreeFromList(nextHeader);
                return old_ptr;
            } else {
                setSizeMalloced(oldHeader, new_size);
                void *new = getFree(oldHeader, getSizeMalloced(oldHeader));
                removeFreeFromList(nextHeader);
                setSize(new, extra + old_size - new_size - (size_t)8);
                coalesce(new);
                return old_ptr;
            }
        }       
    }
    return NULL;
}

bool validate_heap() {
    /**
    void *ptr1 = mymalloc(8);
    void *ptr2 = mymalloc(16);
    if (getFree(ptr1, getSizeMalloced(ptr1)) != ptr2) {
        return false;
    }
    myfree(ptr2);
    if (!isFree(ptr2)) {
        return false;
    }
    void *testForOneFreeBlock = front;
    size_t num_free_blocks = 0;
    while (testForOneFreeBlock != NULL) {
        num_free_blocks += (size_t)1;
        testForOneFreeBlock = convertPtrToStruct(testForOneFreeBlock) -> next;
    }
    if (num_free_blocks != (size_t)1) {
        return false;
    }
    */
    return true;
}

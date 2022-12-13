#include "heap.h"
#include "tested_declarations.h"
#include "rdebug.h"

int heap_setup(void)
{
    memory_manager.memory_size = 0;
    memory_manager.memory_start = custom_sbrk(0);
    memory_manager.first_memory_chunk = NULL;

    if(memory_manager.memory_start == (void*)-1) {
        return -1;
    }

    return 0;
}

void heap_clean(void)
{
    custom_sbrk(memory_manager.memory_size*(-1));
    memory_manager.memory_size = 0;
    memory_manager.first_memory_chunk = NULL;
}

struct memory_chunk_t *check_mem_holes(size_t needed_size)
{
    struct memory_chunk_t *tmp = memory_manager.first_memory_chunk;

    while(tmp->next != NULL) {
        if(tmp->free == 1 && tmp->size >= needed_size) {
            return tmp;
        }

        tmp = tmp->next;
    }

    return NULL;
}

void* heap_malloc(size_t size)
{
    if(size == 0) {
        return NULL;
    }

    if(memory_manager.first_memory_chunk == NULL) {
        memory_manager.memory_size = size + PLOTEK*2 + sizeof(struct memory_chunk_t);

        memory_manager.memory_start = custom_sbrk(size + PLOTEK*2 + sizeof(struct memory_chunk_t));
        if (memory_manager.memory_start == (void *) -1) {
            memory_manager.memory_start = NULL;
            return NULL;
        }

        struct memory_chunk_t *first = (struct memory_chunk_t *) memory_manager.memory_start;
        first->next = NULL;
        first->prev = NULL;
        first->free = 0;
        first->size = size;

        for (int i = 0; i < PLOTEK; ++i) {
            *((char *) first + i + sizeof(struct memory_chunk_t)) = '#';
            *((char *) first + i + size + sizeof(struct memory_chunk_t) + PLOTEK) = '#';
        }

        memory_manager.first_memory_chunk = first;
        return (char *)first + sizeof(struct memory_chunk_t) + PLOTEK;
    }

    //check if any free blocs of sufficient size exist
    struct memory_chunk_t *tmp;
    tmp = check_mem_holes(size);
    if(tmp != NULL) {
        tmp->size = size;
        tmp->free = 0;

        for(int i = 0; i < PLOTEK; i++) {
            *((char*)tmp + sizeof(struct memory_chunk_t) + i) = '#';
            *((char*)tmp + sizeof(struct memory_chunk_t) + i + tmp->size + PLOTEK) = '#';
        }

        return tmp + sizeof(struct memory_chunk_t) + PLOTEK;
    }

    tmp = memory_manager.first_memory_chunk;

    while(tmp->next != NULL) {
        tmp = tmp->next;
    }

    tmp->next = custom_sbrk(size + PLOTEK*2 + sizeof(struct memory_chunk_t));
    if(tmp->next == (void*)-1) {
        memory_manager.first_memory_chunk = NULL;
        return NULL;
    }

    memory_manager.memory_size += size + 2*PLOTEK + sizeof(struct memory_chunk_t);

    struct memory_chunk_t *new = tmp->next;

    new->next = NULL;
    new->prev = tmp;
    new->size = size;
    new->free = 0;

    for (int i = 0; i < PLOTEK; ++i) {
        *((char *) new + i + sizeof(struct memory_chunk_t)) = '#';
        *((char *) new + i + size + sizeof(struct memory_chunk_t) + PLOTEK) = '#';
    }

    tmp->next = new;

    return new + sizeof(struct memory_chunk_t) + PLOTEK;
}

void* heap_calloc(size_t number, size_t size)
{
    if(number*size == 0) {
        return NULL;
    }

    char *tmp = heap_malloc(number*size);
    if(!tmp) {
        return NULL;
    }

    for(size_t i = 0; i < size*number; i++) {
        *(tmp+i) = 0;
    }

    return tmp;
}

void* heap_realloc(void* memblock, size_t count)
{
    return NULL;
}

void heap_free(void* memblock)
{
    if(memblock == NULL) return;

    if(get_pointer_type(memblock) != pointer_valid) {
        return;
    }


    struct memory_chunk_t *tmp = memory_manager.first_memory_chunk;

    while (tmp) {
        if(tmp == (struct memory_chunk_t*)memblock - PLOTEK - sizeof(struct memory_chunk_t)) {
            break;
        }

        tmp = tmp->next;
    }

    tmp->free = 1;
}

size_t heap_get_largest_used_block_size(void)
{
    return 0;
}

enum pointer_type_t get_pointer_type(const void* const pointer)
{
    if(pointer == NULL) {
        return pointer_null;
    }

    if(heap_validate() == 1) {
        return pointer_heap_corrupted;
    }

    struct memory_chunk_t *tmp = memory_manager.first_memory_chunk;

    while(tmp){
        if((char*)tmp <= (char*)pointer && (char*)tmp + sizeof(struct memory_chunk_t) > (char*)pointer) {
            return pointer_control_block;
        }

        if(((struct memory_chunk_t*)pointer - PLOTEK - sizeof(struct memory_chunk_t))->free == 1) {
            return pointer_unallocated;
        }

        tmp = tmp->next;
    }

    return pointer_valid;
}

int heap_validate(void)
{
    struct memory_chunk_t *tmp = memory_manager.first_memory_chunk;
    if(tmp == NULL) {
        return 0;
    }

    do {
        for(int i = 0; i < PLOTEK; i++) {
            if(*((char*)tmp+ sizeof(struct memory_chunk_t) + i) != '#' || *((char*)tmp+ sizeof(struct memory_chunk_t) + i + 4 + tmp->size) != '#') {
                return 1;
            }
        }

        tmp = tmp->next;
    } while(tmp != NULL);

    return 0;
}

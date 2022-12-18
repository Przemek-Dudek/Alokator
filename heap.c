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

void hash_calculate(struct memory_chunk_t *mem)
{
    int val = 0;

    for(size_t i = 0; i < sizeof(struct memory_chunk_t) - sizeof(int); i++) {
        val += *((char*)mem+i);
    }

    mem->CODE = val;
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

        struct memory_chunk_t *tmp = (struct memory_chunk_t *) memory_manager.memory_start;
        tmp->next = NULL;
        tmp->prev = NULL;
        tmp->free = 0;
        tmp->size = size;

        for (int i = 0; i < PLOTEK; ++i) {
            *((char *) tmp + i + sizeof(struct memory_chunk_t)) = '#';
            *((char *) tmp + i + size + sizeof(struct memory_chunk_t) + PLOTEK) = '#';
        }

        memory_manager.first_memory_chunk = tmp;
        hash_calculate(tmp);

        return (char *)tmp + sizeof(struct memory_chunk_t) + PLOTEK;
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

        hash_calculate(tmp);

        return (char *)tmp + sizeof(struct memory_chunk_t) + PLOTEK;
    }


    //########## NEXT BLOCK ##########

    tmp = memory_manager.first_memory_chunk;

    while(tmp->next != NULL) {
        tmp = tmp->next;
    }

    void *new_mem = custom_sbrk(size + PLOTEK*2 + sizeof(struct memory_chunk_t));
    if (new_mem == (void *) -1) {
        new_mem = NULL;
        return NULL;
    }

    memory_manager.memory_size += size + PLOTEK*2 + sizeof(struct memory_chunk_t);

    struct memory_chunk_t *new = (struct memory_chunk_t *) new_mem;
    new->next = NULL;
    new->prev = tmp;
    new->free = 0;
    new->size = size;

    hash_calculate(new);

    for (int i = 0; i < PLOTEK; ++i) {
        *((char *) new + i + sizeof(struct memory_chunk_t)) = '#';
        *((char *) new + i + size + sizeof(struct memory_chunk_t) + PLOTEK) = '#';
    }

    tmp->next = new;

    hash_calculate(tmp);

    return (char *)new + sizeof(struct memory_chunk_t) + PLOTEK;
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

    memset(tmp, 0, size*number);

    return tmp;
}

void* heap_realloc(void* memblock, size_t count)
{
    if(get_pointer_type(memblock) != pointer_valid && memblock != NULL) {
        return NULL;
    }

    if(memblock == NULL) {
        return (char *)heap_malloc(count);
    }

    struct memory_chunk_t *tmp;

    tmp = (struct memory_chunk_t*)memblock - sizeof(struct memory_chunk_t) - PLOTEK;

    if(tmp->size > count) {
        tmp->size = count;

        hash_calculate(tmp);

        return (char *)tmp + sizeof(struct memory_chunk_t) + PLOTEK;
    }

    //sprawdzic nast. blok



    struct memory_chunk_t *new = heap_malloc(count);

    if(!new) {
        return NULL;
    }

    for(size_t i = 0; i < tmp->size; i++) {
        *(new+i) = *(tmp+i);
    }

    return new;
}

void heap_connect(struct memory_chunk_t *mem)
{
    struct memory_chunk_t *tmp = (struct memory_chunk_t*)mem;

    while(mem) {
        if(mem->free == 1) {
            tmp = mem;
        }

        mem = mem->next;

        if(tmp->free && mem && mem->free == 1) {
            tmp->size += mem->size + sizeof(struct memory_chunk_t) + PLOTEK*2;

            tmp->next = mem->next;

            hash_calculate(tmp);

            if(mem->next) {
                mem->next->prev = tmp;
                hash_calculate(mem);
            }
        } else {
            tmp = tmp->next;

            hash_calculate(tmp);
        }

        if(mem->next == NULL) {
            break;
        }

        mem = tmp;
    }
}

void check_destroy(struct memory_chunk_t *mem)
{
    int flag = 1;

    while(mem != NULL) {
        if(!mem->free) {
            flag = 0;
        }

        mem = mem->next;
    }

    if(flag) {
        heap_clean();
    }
}

void heap_free(void* memblock)
{
    if(memblock == NULL) return;

    if(get_pointer_type(memblock) != pointer_valid) {
        return;
    }


    struct memory_chunk_t *tmp = memory_manager.first_memory_chunk;

    while (tmp != NULL) {
        if((char*)tmp +PLOTEK + sizeof(struct memory_chunk_t) == (char*)memblock  ) {
            tmp->free = 1;

            hash_calculate(tmp);

            check_destroy(tmp);

            heap_connect((struct memory_chunk_t*)memory_manager.first_memory_chunk);

            /*if(tmp->next == NULL && tmp->free) {
                if(tmp->prev != NULL) {
                    tmp->prev->next = NULL;
                }
            }*/

            break;
        }

        tmp = tmp->next;
    }
}

size_t heap_get_largest_used_block_size(void)
{
    if(memory_manager.memory_size == 0 || memory_manager.first_memory_chunk == NULL) {
        return 0;
    }

    if(heap_validate() != 0) {
        return 0;
    }

    struct memory_chunk_t *tmp = memory_manager.first_memory_chunk;

    size_t res = 0;

    while(tmp != NULL) {
        if(tmp->size > res && !tmp->free) res = tmp->size;

        tmp = tmp->next;
    }

    return res;
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
        if(!tmp->free) {
            if((char*)tmp <= (char*)pointer && (char*)tmp + sizeof(struct memory_chunk_t) > (char*)pointer) {
                return pointer_control_block;
            }

            if((char*)pointer >= (char*)tmp + sizeof(struct memory_chunk_t) && (char*)pointer < (char*)tmp + sizeof(struct memory_chunk_t) + PLOTEK) {
                return pointer_inside_fences;
            }

            if((char*)pointer == (char*)tmp + sizeof(struct memory_chunk_t) + PLOTEK) {
                return pointer_valid;
            }

            if((char*)pointer > (char*)tmp + sizeof(struct memory_chunk_t) + PLOTEK && (char*)pointer < (char*)tmp + sizeof(struct memory_chunk_t) + PLOTEK + tmp->size) {
                return pointer_inside_data_block;
            }

            if((char*)pointer >= (char*)tmp + sizeof(struct memory_chunk_t) + PLOTEK + tmp->size && (char*)pointer < (char*)tmp + sizeof(struct memory_chunk_t) + 2*PLOTEK + tmp->size) {
                return pointer_inside_fences;
            }
        }

        tmp = tmp->next;
    }

    return pointer_unallocated;
}

int hash_check(struct memory_chunk_t *mem)
{
    int val = 0;

    for(size_t i = 0; i < sizeof(struct memory_chunk_t) - sizeof(int); i++) {
        val += *((char*)mem+i);
    }

    if(val != mem->CODE) {
        return 1;
    }

    return 0;
}

int heap_validate(void)
{
    struct memory_chunk_t *tmp = memory_manager.first_memory_chunk;
    if(tmp == NULL) {
        return 0;
    }


    do {
        for(int i = 0; i < PLOTEK; i++) {
            if(*((char*)tmp + sizeof(struct memory_chunk_t) + i) != '#' || *((char*)tmp+ sizeof(struct memory_chunk_t) + i + 4 + tmp->size) != '#') {
                return 1;
            }
        }

        if(hash_check(tmp)) {
            return 3;
        }

        tmp = tmp->next;
    } while(tmp != NULL);

    return 0;
}

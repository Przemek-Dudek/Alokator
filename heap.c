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
    memory_manager.memory_start = NULL;
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

    if(memory_manager.memory_start == NULL) {
        return NULL;
    }

    if(count == 0) {
        heap_free(memblock);

        return NULL;
    }

    if(memblock == NULL) {
        char *tmp = heap_malloc(count);
        if(!tmp) {
            return NULL;
        }

        return tmp;
    }

    struct memory_chunk_t *tmp;

    tmp = (struct memory_chunk_t*)((char*)memblock - sizeof(struct memory_chunk_t) - PLOTEK);

    if(tmp->next != NULL) {
        size_t size = (char*)tmp->next - (char*)tmp - sizeof(struct memory_chunk_t) - PLOTEK*2;
        if(size > tmp->size) {
            tmp->size = size;
        }
    }

    if(tmp->size >= count) {
        tmp->size = count;

        memset((char*)memblock + tmp->size, '#', 4);

        hash_calculate(tmp);

        return memblock;
    }

    if(tmp->size == count) {
        return memblock;
    }

    //spr. czy nast blok == NULL

    if(tmp->next == NULL) {
        void *new_mem = custom_sbrk(count - tmp->size);
        if (new_mem == (void *) -1) {
            new_mem = NULL;
            return NULL;
        }

        memory_manager.memory_size += count - tmp->size;

        tmp->size = count;

        memset((char*)memblock+tmp->size, '#', 4);

        hash_calculate(tmp);

        return memblock;
    }

    //sprawdzic nast. blok
    heap_connect(memory_manager.first_memory_chunk);

    if(tmp->next && tmp->next->free && tmp->size+tmp->next->size + sizeof(struct memory_chunk_t) + PLOTEK*2 >= count) {
        tmp->next = tmp->next->next;
        tmp->size = count;

        hash_calculate(tmp);


        if(tmp->next->next) {
            tmp->next->next->prev = tmp;

            hash_calculate(tmp->next->next);
        }

        memset((char*)memblock+tmp->size, '#', 4);

        return memblock;
    }


    char *new = heap_malloc(count);

    if(!new) {
        return NULL;
    }

    for(size_t i = 0; i < tmp->size; i++) {
        *((char*)new+i) = *((char*)tmp + PLOTEK + sizeof(struct memory_chunk_t)+i);
    }

    heap_free((char *)tmp + PLOTEK + sizeof(struct memory_chunk_t));

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
        custom_sbrk(memory_manager.memory_size*(-1));
        memory_manager.memory_size = 0;
        memory_manager.first_memory_chunk = NULL;
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

            if(tmp->next != NULL) {
                size_t size = (char*)tmp->next - (char*)tmp - sizeof(struct memory_chunk_t) - PLOTEK*2;
                if(size > tmp->size) {
                    tmp->size = size;
                }
            }

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
    if(memory_manager.memory_start == NULL && memory_manager.first_memory_chunk == NULL) {
        return 2;
    }

    struct memory_chunk_t *tmp = memory_manager.first_memory_chunk;
    if(tmp == NULL) {
        return 0;
    }

    do {
        if(hash_check(tmp)) {
            return 3;
        }

        for(int i = 0; i < PLOTEK; i++) {
            if(*((char*)tmp + sizeof(struct memory_chunk_t) + i) != '#' || *((char*)tmp+ sizeof(struct memory_chunk_t) + i + 4 + tmp->size) != '#') {
                return 1;
            }
        }

        tmp = tmp->next;
    } while(tmp != NULL);

    return 0;
}

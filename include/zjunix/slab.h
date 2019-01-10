#ifndef _ZJUNIX_SLAB_H
#define _ZJUNIX_SLAB_H

#include <zjunix/list.h>
#include <zjunix/buddy.h>
#include <zjunix/memory.h>

#define SIZE_INT 4
#define SLAB_AVAILABLE 0x0
#define SLAB_USED 0xff

#define KMEM_ADDR(PAGE, BASE) ((((PAGE) - (BASE)) << PAGE_SHIFT) | 0x80000000)

/*
 * slab_head makes the allocation accessible from end_ptr to the end of the page
 * @end_ptr : points to the head of the rest of the page
 * @nr_objs : keeps the numbers of memory segments that has been allocated
 */
typedef struct slab_head {
    void *end_ptr;
    unsigned int nr_objs;
    boolean_t was_full;
} slab_t;

/*
 * slab pages is chained in this struct
 * @partial keeps the list of un-totally-allocated pages
 * @full keeps the list of totally-allocated pages
 */
typedef struct kmem_cache_node {
    struct list_head partial;
    struct list_head full;
} kmem_cache_node_t;

/*
 * current being allocated page unit
 */
typedef struct kmem_cache_cpu {
    void* *freelist;  // points to the free-space head addr inside current page
    page_t *page;
} kmem_cache_cpu_t;

// TODO: 补全函数声明

typedef struct kmem_cache_struct{
    unsigned int size;
    unsigned int objsize;
    unsigned int offset;
    struct kmem_cache_node node;
    struct kmem_cache_cpu cpu;
    char* name;
} kmem_cache_t;

// extern struct kmem_cache kmalloc_caches[PAGE_SHIFT];
extern void init_slab();
extern void *slab_kmalloc(unsigned int size);
extern void slab_kfree(void *obj);

#endif

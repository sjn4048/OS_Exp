#include <arch.h>
#include <driver/vga.h>
#include <zjunix/slab.h>
#include <zjunix/utils.h>
#include <zjunix/log.h>

/*
 * one list of PAGE_SHIFT(12) possbile memory size
 * 96, 192, 8, 16, 32, 64, 128, 256, 512, 1024, (2 undefined)
 * in current stage, set (2 undefined) to be (4, 2048)
 */
kmem_cache kmalloc_caches[PAGE_SHIFT];

static unsigned int size_kmem_cache[PAGE_SHIFT] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 96, 192, 1536};

// initialize struct kmem_cache_cpu
void init_kmem_cpu(struct kmem_cache_cpu *kcpu)
{
    kcpu->page = 0;
    kcpu->freeobj = 0;
}

void init_kmem_node(struct kmem_cache_node *knode)
{
    INIT_LIST_HEAD(&(knode->full));
    INIT_LIST_HEAD(&(knode->partial));
}

void init_each_slab(kmem_cache *cache, unsigned int size)
{
    cache->objsize = (size + (SIZE_INT - 1)) & ~(SIZE_INT - 1);
    // TODO: ?
    cache->size = cache->objsize + sizeof(void *);
    cache->offset = cache->size;
    init_kmem_cpu(&cache->cpu);
    init_kmem_node(&cache->node);
}

void init_slab()
{
    // sort the size_kmem_cache. Use bubble sort as scale is small.
    for (int i = 0; i < PAGE_SHIFT - 1; i++)
    {
        int isSorted = 1;
        for (int j = 0; j < PAGE_SHIFT - 1 - i; j++)
        {
            if (size_kmem_cache[j] > size_kmem_cache[j + 1])
            {
                int temp = size_kmem_cache[j];
                size_kmem_cache[j] = size_kmem_cache[j + 1];
                size_kmem_cache[j + 1] = temp;
                isSorted = 0;
            }
        }

        if (isSorted)
        {
            break;
        }
    }

    for (int i = 0; i < PAGE_SHIFT; i++)
    {
        init_each_slab(kmalloc_caches + i, size_kmem_cache[i]);
    }
#ifdef SLAB_DEBUG
    kernel_printf("Setup Slub success:\n");
    kernel_printf("\tCurrent slab cache size list:\n\t");
    for (i = 0; i < PAGE_SHIFT; i++)
    {
        kernel_printf("%x %x ", kmalloc_caches[i].objsize, kmalloc_caches + i);
    }
    kernel_printf("\n");
#endif // SLAB_DEBUG
}

// ATTENTION: sl_objs is the reuse of bplevel
// ATTENTION: slabp must be set right to add itself to reach the end of the page
// 		e.g. if size = 96 then 4096 / 96 = .. .. 64 then slabp starts at
// 64
// TODO: review
void format_slabpage(kmem_cache *cache, struct page *page) {
    unsigned char *moffset = (unsigned char *)KMEM_ADDR(page, pages);  // physical addr
    struct slab_head *s_head = (struct slab_head *)moffset;
    unsigned int *ptr;
    unsigned int remaining = 1 << PAGE_SHIFT;

    set_flag(page, _PAGE_SLAB);
    do
    {
        ptr = (unsigned int *)(moffset + cache->offset);
        moffset += cache->size;
        *ptr = (unsigned int)moffset;
        remaining -= cache->size;
    } while (remaining >= cache->size);

    *ptr = (unsigned int)moffset & ~((1 << PAGE_SHIFT) - 1);
    s_head->end_ptr = ptr;
    s_head->nr_objs = 0;

    cache->cpu.page = page;
    cache->cpu.freeobj = (void **)(*ptr + cache->offset);
    page->virtual = (void *)cache;
    page->slabp = (unsigned int)(*cache->cpu.freeobj);
}


// TODO: review
void *slab_alloc(kmem_cache *cache)
{
    slab *s_head;
    void *object = 0;

    // check if there is free
    if (cache->cpu.freeobj != 0)
    {
        object = *cache->cpu.freeobj;
    }
slalloc_check:
    // check if the freeobj is in the boundary situation
    if (is_bound((unsigned int)object, 1 << PAGE_SHIFT))
    {
        // if the page is full
        if (cache->cpu.page != 0)
        {
            list_add_tail(&cache->cpu.page->list, &cache->node.full);
        }

        if (list_empty(&cache->node.partial))
        {
            // get one page
            struct page *newpage = __alloc_pages(0);
            if (newpage == 0)
            {
                // fail due to not enough system memory
                kernel_printf("[ERROR] Slab failed due to system run out\n");
                while (1)
                    ;
            }
#ifdef SLAB_DEBUG
            kernel_printf("\tNew page, index: %x \n", newpage - pages);
#endif // ! SLAB_DEBUG
            format_slabpage(cache, newpage);
            object = *cache->cpu.freeobj;
            // CHANGE: removed a <goto>
        }
        // get the header of the cpu.page(struct page)
        cache->cpu.page = container_of(cache->node.partial.next, struct page, list);
        list_del(cache->node.partial.next);
        object = (void *)(cache->cpu.page->slabp);
        cache->cpu.freeobj = (void **)((unsigned char *)object + cache->offset);
        goto slalloc_check;
    }
slalloc_normal:
    cache->cpu.freeobj = (void **)((unsigned char *)object + cache->offset);
    cache->cpu.page->slabp = (unsigned int)(*(cache->cpu.freeobj));
    s_head = (struct slab_head *)KMEM_ADDR(cache->cpu.page, pages);
    ++(s_head->nr_objs);
slalloc_end:
    // slab may be full after this allocation
    if (is_bound(cache->cpu.page->slabp, 1 << PAGE_SHIFT))
    {
        list_add_tail(&(cache->cpu.page->list), &(cache->node.full));
        init_kmem_cpu(&(cache->cpu));
    }
    return object;
}

void free_slab(kmem_cache *cache, void *object)
{
    struct page *opage = pages + ((unsigned int)object >> PAGE_SHIFT);
    slab *s_head = (slab *)KMEM_ADDR(opage, pages);

    if (s_head->nr_objs == 0)
    {
        kernel_printf("[ERROR] Slab_free error: head does not exist.\n");
        while (1)
            ;
    }

    unsigned int *ptr = (unsigned int *)((unsigned char *)object + cache->offset);
    *ptr = *(unsigned int *)(s_head->end_ptr);
    *(unsigned int *)s_head->end_ptr = (unsigned int)object;
    s_head->nr_objs--;

    if (list_empty(&opage->list))
        return;

    if (s_head->nr_objs == 0)
    {
        free_pages(object, 0);
        return;
    }

    list_del_init(&opage->list);
    list_add_tail(&opage->list, &cache->node.partial);
}

// find the best-fit slab system for (size)
unsigned int find_slab(unsigned int size)
{
    for (unsigned int i = 0; i < PAGE_SHIFT; i++)
    {
        // find first slab that can contain it, as slabs has been sorted.
        if (kmalloc_caches[i].objsize >= size)
        {
            return i;
        }
    }
}

void *slab_kmalloc(unsigned int size)
{
    struct kmem_cache *cache;
    unsigned int bf_index;

    if (size == 0)
    {
        return 0;
    }

    // if the size larger than the max size of slab system, then call buddy to
    // solve this
    if (size > kmalloc_caches[PAGE_SHIFT - 1].objsize)
    {
        size += (1 << PAGE_SHIFT) - 1;
        size &= ~((1 << PAGE_SHIFT) - 1);
        return (void *)(KERNEL_ENTRY | (unsigned int)alloc_pages(size >> PAGE_SHIFT));
    }

    bf_index = find_slab(size);
    if (bf_index >= PAGE_SHIFT) {
        kernel_printf("ERROR: No available slab\n");
        while (1)
            ;
    }
    return (void *)(KERNEL_ENTRY | (unsigned int)slab_alloc(&kmalloc_caches[bf_index]));
}

void slab_kfree(void *obj)
{
    struct page *page;

    obj = (void *)((unsigned int)obj & (~KERNEL_ENTRY));
    page = pages + ((unsigned int)obj >> PAGE_SHIFT);
    if (page->flag != _PAGE_SLAB)
        return free_pages((void *)((unsigned int)obj & ~((1 << PAGE_SHIFT) - 1)), page->bplevel);

    return free_slab(page->virtual, obj);
}

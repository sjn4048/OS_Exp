#include <arch.h>
#include <driver/vga.h>
#include <zjunix/slab.h>
#include <zjunix/utils.h>
#include <zjunix/log.h>
#include <zjunix/memory.h>

/*
 * one list of PAGE_SHIFT(12) possbile memory size
 * 48, 96, 192, 1536, 8, 16, 32, 64, 128, 256, 512, 1024
 */
kmem_cache_t slab_kmem_caches[PAGE_SHIFT];

static size_t slab_kmem_cache_sizes[PAGE_SHIFT] = {48, 96, 192, 1536, 8, 16, 32, 64, 128, 256, 512, 1024};

void print_each_cache()
{
    for (int i = 0; i < PAGE_SHIFT; i++)
    {
        kernel_printf("cache %d:", i);
        kernel_printf("%d %d %d\n",
                      slab_kmem_caches[i].size, slab_kmem_caches[i].offset, slab_kmem_caches[i].objsize);
        kernel_getchar();
    }
}

// initialize struct kmem_cache_cpu
void init_kmem_cpu(kmem_cache_cpu_t *kcpu)
{
    kcpu->page = NULL;
    kcpu->freelist = 0;
}

void init_kmem_node(kmem_cache_node_t *knode)
{
    INIT_LIST_HEAD(&knode->full);
    INIT_LIST_HEAD(&knode->partial);
}

void init_each_slab(kmem_cache_t *cache, size_t size)
{
    cache->objsize = UPPER_ALLIGN(size, SIZE_INT);
    cache->size = cache->objsize + sizeof(void *);
    cache->offset = cache->size;
    init_kmem_cpu(&cache->cpu);
    init_kmem_node(&cache->node);
}

void init_slab()
{
    // sort the slab_kmem_cache_sizes. Use bubble sort as scale is small.
    for (int i = 0; i < PAGE_SHIFT - 1; i++)
    {
        int isSorted = 1;
        for (int j = 0; j < PAGE_SHIFT - 1 - i; j++)
        {
            if (slab_kmem_cache_sizes[j] > slab_kmem_cache_sizes[j + 1])
            {
                int temp = slab_kmem_cache_sizes[j];
                slab_kmem_cache_sizes[j] = slab_kmem_cache_sizes[j + 1];
                slab_kmem_cache_sizes[j + 1] = temp;
                isSorted = 0;
            }
        }

        if (isSorted)
        {
            break;
        }
    }
#ifdef SLAB_DEBUG
    kernel_printf("In init_alab:\n");
    kernel_printf("\tSort slab_kmem_cache_sizes to:");
    for (int i = 0; i < PAGE_SHIFT; i++)
    {
        kernel_printf("%d ", slab_kmem_cache_sizes[i]);
        kernel_printf("\n");
    }
#endif // SLAB_DEBUG

    for (int i = 0; i < PAGE_SHIFT; i++)
    {
        init_each_slab(slab_kmem_caches + i, slab_kmem_cache_sizes[i]);
    }
#ifdef SLAB_DEBUG
    kernel_printf("Setup Slab success:\n");
    kernel_printf("\tCurrent slab cache size list:\n\t");
    for (int i = 0; i < PAGE_SHIFT; i++)
    {
        kernel_printf("%x %x ", slab_kmem_caches[i].objsize, slab_kmem_caches + i);
    }
    kernel_printf("\n");
#endif // SLAB_DEBUG
}

// format the slabpage.
void format_slab_page(kmem_cache_t *cache, page_t *page)
{
    unsigned char *page_addr = (unsigned char *)KMEM_ADDR(page, pages); // physical addr
    slab_t *s_head = (slab_t *)page_addr;
    void *ptr = page_addr + sizeof(slab_t);

    set_flag(page, _PAGE_SLAB);
#ifdef SLAB_DEBUG
    kernel_printf("\tIn format_slab_page, page %x after set_flag: %x", page, page->flag);
#endif // ! SLAB_DEBUG
    s_head->end_ptr = ptr;
    s_head->nr_objs = 0;
    s_head->was_full = False;

    cache->cpu.page = page;
    cache->cpu.freelist = (void **)((unsigned int)ptr + cache->offset);
    page->virtual = (void *)cache;
    page->slabp = 0; //(unsigned int)(*cache->cpu.freelist);
#ifdef SLAB_DEBUG
    kernel_printf("In format_slab_page:\n\t page=%x, freelist=%x->%x->%x, virtual=%x, slabp=%x\n", (unsigned int)page_addr, cache->cpu.freelist, *cache->cpu.freelist, *cache->cpu.freelist, page->virtual, page->slabp);
#endif // ! SLAB_DEBUG
}

void *slab_alloc(kmem_cache_t *cache)
{
    page_t *cur_page = NULL;
    void *ret = NULL;
    slab_t *slab_head = NULL;

    // current page of CPU is not null
    if (cache->cpu.page != NULL)
    {
        cur_page = cache->cpu.page;
        slab_head = (slab_t *)KMEM_ADDR(cur_page, pages);
    FreeList:
        if (cur_page->slabp != NULL)
        {
            // if current slabp is not NULL
            ret = (void *)cur_page->slabp;
            cur_page->slabp = *(size_t *)cur_page->slabp;
            slab_head->nr_objs++;
#ifdef SLAB_DEBUG
            kernel_printf("From Free-list\nnr_objs:%d\rret:%x\tnew slabp:%x\n",
                          slab_head->nr_objs, ret, cur_page->slabp);
            // //kernel_getchar();
#endif // ! SLAB_DEBUG
            return ret;
        }
        else if (slab_head->was_full == False)
        {
        PageNotFull:
            ret = slab_head->end_ptr;
            slab_head->end_ptr = (char *)ret + cache->size;
            slab_head->nr_objs++;

            // high risk here.
            if ((unsigned int)slab_head->end_ptr - (unsigned int)slab_head + cache->size >= PAGE_SIZE)
            {
                slab_head->was_full = True;
                list_add_tail(&cur_page->list, &cache->node.full);
#ifdef SLAB_DEBUG
                kernel_printf("Become full now. total size: %d. end_ptr: %x, head: %x, cache_size:\n", (unsigned int)slab_head->end_ptr - (unsigned int)slab_head + cache->size, slab_head->end_ptr, slab_head, cache->size);
                // //kernel_getchar();
#endif // ! SLAB_DEBUG
            }
#ifdef SLAB_DEBUG
            kernel_printf("Page not full yet\nnr_objs:%d\tobject:%x\tend_ptr:%x\n",
                          slab_head->nr_objs, ret, slab_head->end_ptr);
            // //kernel_getchar();
#endif // ! SLAB_DEBUG
            return ret;
        }
    }

    // page full!
#ifdef SLAB_DEBUG
    kernel_printf("Page full or not exist.\n");
    // //kernel_getchar();
#endif // ! SLAB_DEBUG

    if (list_empty(&cache->node.partial))
    {
        // cur_page: 指向page_t结构的指针！不是page的内存地址！
        cur_page = __alloc_pages(0);
        if (cur_page == NULL)
        {
            // fail due to not enough system memory
            kernel_printf("[ERROR] Slab failed due to system run out\n");
            while (1)
                ;
        }
#ifdef SLAB_DEBUG
        kernel_printf("\tnewpage: %x, pages: %x, index: %x \n", cur_page, pages, cur_page - pages);
        // kernel_printf("\tnew page, index: %x \n", cur_page - pages);
#endif // ! SLAB_DEBUG

        // using standard format to shape the new-allocated page,
        // set the new page to be cpu.page
        format_slab_page(cache, cur_page);
        slab_head = (slab_t *)KMEM_ADDR(cur_page, pages);
        goto PageNotFull;
    }
    else
    {
        // found a partial page
#ifdef SLAB_DEBUG
        kernel_printf("Found partial page\n");
#endif
        cache->cpu.page = container_of(cache->node.partial.next, page_t, list);
        cur_page = cache->cpu.page;
        list_del(cache->node.partial.next);
        slab_head = (slab_t *)KMEM_ADDR(cur_page, pages);
        goto FreeList;
    }
}

// TODO: review what's wrong here.
void *slab_alloc_old(kmem_cache_t *cache)
{
    slab_t *s_head;
    void *object = NULL;

    // check if there is free
    if (cache->cpu.freelist != NULL)
    {
        // let object be the freelist page.
        object = *cache->cpu.freelist;
    }
slalloc_check:
    // check if the freelist is in the boundary situation
#ifdef SLAB_DEBUG
    // kernel_printf("\tobject: %x\n", (unsigned int)object);
#endif // ! SLAB_DEBUG
    if (is_bound((unsigned int)object, PAGE_SIZE))
    {
        // if the page is full
        if (cache->cpu.page != NULL)
        {
            list_add_tail(&cache->cpu.page->list, &cache->node.full);
        }

        // if list is empty now
        if (list_empty(&cache->node.partial))
        {
            // get one page
            page_t *newpage = __alloc_pages(0);
            if (newpage == 0)
            {
                // fail due to not enough system memory
                kernel_printf("[ERROR] Slab failed due to system run out\n");
                while (1)
                    ;
            }
#ifdef SLAB_DEBUG
            kernel_printf("\tnewpage: %x, pages: %x, index: %x \n\tCheck the newly allocated page [1]\n.", newpage, pages, newpage - pages);
#endif // ! SLAB_DEBUG
            format_slab_page(cache, newpage);
            object = *cache->cpu.freelist;

            goto slalloc_check;
        }
        // get the header of the cpu.page
        cache->cpu.page = container_of(cache->node.partial.next, page_t, list);
        list_del(cache->node.partial.next);
        object = (void *)(cache->cpu.page->slabp);
        cache->cpu.freelist = (void **)((unsigned char *)object + cache->offset);
        goto slalloc_check;
    }
slalloc_normal:
    cache->cpu.freelist = (void **)((unsigned char *)object + cache->offset);
    cache->cpu.page->slabp = (unsigned int)(*cache->cpu.freelist);
    s_head = (slab_t *)KMEM_ADDR(cache->cpu.page, pages);
    s_head->nr_objs++;
slalloc_end:
    // slab may be full after this allocation
    if (is_bound(cache->cpu.page->slabp, PAGE_SIZE))
    {
        list_add_tail(&cache->cpu.page->list, &cache->node.full);
        init_kmem_cpu(&cache->cpu);
    }
#ifdef SLAB_DEBUG
    kernel_printf("\tslab_alloc ret: %x\n", object);
#endif // ! SLAB_DEBUG
    return object;
}

void free_slab(kmem_cache_t *cache, void *object)
{
    page_t *target_page = pages + (((unsigned int)object - KERNEL_ENTRY) >> PAGE_SHIFT);
    slab_t *s_head = (slab_t *)KMEM_ADDR(target_page, pages);

    if (s_head->nr_objs == 0)
    {
        kernel_printf("[ERROR] Slab_free error: head does not exist.\n");
        kernel_printf("slab_head:%x, object: %x, cache: %x", s_head, object,
                      cache);

        while (1)
            ;
    }

    boolean_t full = (target_page->slabp == NULL) && s_head->was_full;
    *(unsigned int *)object = target_page->slabp;
    target_page->slabp = (unsigned int)object;
    s_head->nr_objs--;

#ifdef SLAB_DEBUG
    kernel_printf("nr_objs:%d\tslabp:%x\n", s_head->nr_objs, target_page->slabp);
    // //kernel_getchar();
#endif //SLAB_DEBUG

    if (list_empty(&target_page->list))
    {
        return;
    }

    // if the head is empty, free it.
    if (s_head->nr_objs == 0)
    {
        list_del_init(&target_page->list);
        free_pages(target_page, 0);
        return;
    }
    if (full)
    {
        // Here! We cannot use list_del_init because it has already been deleted!
        INIT_LIST_HEAD(&target_page->list);
        list_add_tail(&target_page->list, &cache->node.partial);
    }
}
// find the best-fit slab system for (size)
off_t find_slab(size_t size)
{
    // if size is too big:
    for (off_t i = 0; i < PAGE_SHIFT; i++)
    {
        // find first slab that can contain it, as slabs has been sorted.
        if (slab_kmem_caches[i].objsize >= size && slab_kmem_caches[i].objsize <= (PAGE_SIZE >> 1))
        {
#ifdef SLAB_DEBUG
            //kernel_printf("\tIn find_slab(): found cache[%d]=%d for size %d\n", i, slab_kmem_caches[i].objsize, size);
            //kernel_getchar();
#endif // ! SLAB_DEBUG
            return i;
        }
    }
#ifdef SLAB_DEBUG
    kernel_printf("In find_slab(): no valid slab is found.\n");
    //kernel_getchar();
#endif // ! SLAB_DEBUG
    return PAGE_SHIFT;
}

void *slab_kmalloc(size_t size)
{
#ifdef SLAB_DEBUG
    kernel_printf("\tslab_kmalloc: size: %d\n", size);
    //kernel_getchar();
#endif // ! SLAB_DEBUG

    if (size == 0)
    {
        return 0;
    }

    void *ret;

    // if the size larger than the max size of slab system, then call buddy to
    // solve this
    if (size > slab_kmem_caches[PAGE_SHIFT - 1].objsize)
    {
#ifdef SLAB_DEBUG
        kernel_printf("\tCall buddy to solve this.\n", size);
        //kernel_getchar();
#endif // ! SLAB_DEBUG
        size = UPPER_ALLIGN(size, PAGE_SIZE);
        ret = (void *)(KERNEL_ENTRY | (unsigned int)alloc_pages(size >> PAGE_SHIFT));
#ifdef SLAB_DEBUG
        print_each_cache();
#endif
        return ret;
    }

    size_t slab_idx = find_slab(size);
    if (slab_idx >= PAGE_SHIFT)
    {
        kernel_printf("ERROR: No available slab as container index exceeds maximum index.\n");
        while (1)
            ;
    }
#ifdef SLAB_DEBUG
    kernel_printf("\tslab: returns: %x\n", (void *)(KERNEL_ENTRY | (unsigned int)slab_alloc(&slab_kmem_caches[slab_idx])));
    //kernel_getchar();
#endif // ! SLAB_DEBUG
#ifdef SLAB_DEBUG
    print_each_cache();
#endif
    return (void *)(KERNEL_ENTRY | (unsigned int)slab_alloc(&slab_kmem_caches[slab_idx]));
}

void slab_kfree(void *obj)
{
    page_t *page;
    page = pages + (((unsigned int)obj - KERNEL_ENTRY) >> PAGE_SHIFT);
#ifdef SLAB_DEBUG
    kernel_printf("\tobj: %x, page: %x, bplevel: %d\n", obj, page, page->bplevel);
    //kernel_getchar();
#endif // ! SLAB_DEBUG
    if (!has_flag(page, _PAGE_SLAB))
    {
#ifdef SLAB_DEBUG
        kernel_printf("\tFlag: %x, call buddy to solve this.\n", page->flag);
        // //kernel_getchar();
#endif // ! SLAB_DEBUG
        free_pages((void *)((unsigned int)obj & PAGE_MASK), page->bplevel);
#ifdef SLAB_DEBUG
        print_each_cache();
#endif
        return;
    }

    free_slab(page->virtual, obj);
#ifdef SLAB_DEBUG
    print_each_cache();
#endif
    return;
}
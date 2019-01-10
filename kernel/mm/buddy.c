#include <arch.h>
#include <driver/vga.h>
#include <zjunix/bootmm.h>
#include <zjunix/buddy.h>
#include <zjunix/list.h>
#include <zjunix/lock.h>
#include <zjunix/utils.h>
#include <zjunix/memory.h>

// initialize all memory with page struct
void init_pages(size_t start_pfn, size_t end_pfn)
{
    for (size_t i = start_pfn; i < end_pfn; i++)
    {
        set_flag(&pages[i], _PAGE_RESERVED);
        pages[i].reference = 1;
        pages[i].virtual = (void *)(-1);
        pages[i].bplevel = 0;
        pages[i].slabp = NULL;
        // initially, the free space is the whole page
        INIT_LIST_HEAD(&pages[i].list);
    }
}

void init_buddy()
{
    pages = (page_t *)bootmm_alloc(sizeof(page_t) * bmm.max_pfn, _MM_KERNEL, 1 << PAGE_SHIFT);
    if (pages == NULL)
    {
        // if bootmm fails to provide memory
        kernel_printf("\nERROR : bootmm_alloc failed!\nInit buddy system failed!\n");
        while (1)
            ;
    }

    init_pages(0, bmm.max_pfn);

    off_t kernel_end_page = 0;
    for (size_t i = 0; i < bmm.info_cnt; i++)
    {
        if (bmm.info[i].end > kernel_end_page)
        {
            kernel_end_page = bmm.info[i].end;
        }
    }
    kernel_end_page >>= PAGE_SHIFT;

    // the pages that bootmm using cannot be merged into buddy_sys
    buddy.buddy_start_pfn = UPPER_ALLIGN(kernel_end_page, 1 << MAX_BUDDY_ORDER);
    buddy.buddy_end_pfn = bmm.max_pfn & ~((1 << MAX_BUDDY_ORDER) - 1); // remain 2 pages for I/O

    // init freelists of all bplevels
    for (size_t i = 0; i <= MAX_BUDDY_ORDER; i++)
    {
        buddy.freelist[i].nr_free = 0;
        INIT_LIST_HEAD(&buddy.freelist[i].free_head);
    }
    buddy.start_page = pages + buddy.buddy_start_pfn;
    init_lock(&buddy.lock);

    for (size_t i = buddy.buddy_start_pfn; i < buddy.buddy_end_pfn; i++)
    {
        __free_pages(&pages[i], 0);
    }
}

void __free_pages(page_t *page, size_t level)
{
    /* page_idx -> the current page
     * bgroup_idx -> the buddy group that current page is in
     */
    size_t page_idx, bgroup_idx;
    size_t combined_idx, tmp;
    page_t *bgroup_page;

    // dec_ref(pbpage, 1);
    // if(pbpage->reference)
    //	return;

    lockup(&buddy.lock);

    // complier do the sizeof(struct) operation, and now page_idx is the index
    page_idx = page - buddy.start_page;
    while (level < MAX_BUDDY_ORDER)
    {
        bgroup_idx = page_idx ^ (1 << level);
        bgroup_page = page + (bgroup_idx - page_idx);
        // kernel_printf("group%x %x\n", (page_idx), bgroup_idx);
        if (!_is_same_bplevel(bgroup_page, level))
        {
#ifdef BUDDY_DEBUG
            kernel_printf("%x %x\n", bgroup_page->bplevel, level);
            kernel_getchar();
#endif
            break;
        }

        if (bgroup_page->flag != PAGE_FREE)
        {
#ifdef BUDDY_DEBUG
            kernel_printf("Pair page %x has been allocated.\n", bgroup_page);
#endif
            break; // Its pair has been allocated or reserved
        }

        list_del_init(&bgroup_page->list);
        buddy.freelist[level].nr_free--;
        combined_idx = bgroup_idx & page_idx;
        if (combined_idx == bgroup_idx)
        {
            set_bplevel(bgroup_page, -1);
        }
        else
        {
            set_bplevel(page, -1);
        }
        page += (combined_idx - page_idx);
        page_idx = combined_idx;
        level++;
    }
    set_bplevel(page, level);
    set_flag(page, PAGE_FREE);
    list_add(&page->list, &buddy.freelist[level].free_head);
    buddy.freelist[level].nr_free++;
#ifdef BUDDY_DEBUG
    kernel_printf("v%x__addto__%x\n", &page->list, &buddy.freelist[level].free_head);
    kernel_getchar();
#endif // BUDDY_DEBUG
    unlock(&buddy.lock);
}

page_t *__alloc_pages(size_t level)
{
    lockup(&buddy.lock);

    for (size_t current_order = level; current_order <= MAX_BUDDY_ORDER; current_order++)
    {
#ifdef BUDDY_DEBUG
        kernel_printf("\tcurrent_order: %d\n", current_order);
        kernel_getchar();
#endif // BUDDY_DEBUG
        freelist_t *free = buddy.freelist + current_order;
        if (!list_empty(&free->free_head))
        {
            page_t *page = container_of(free->free_head.next, page_t, list);
#ifdef BUDDY_DEBUG
            kernel_printf("\tif-page: %x\n", page);
            kernel_getchar();
#endif // BUDDY_DEBUG
            list_del_init(&page->list);
            set_bplevel(page, level);
            set_flag(page, _PAGE_ALLOCED);
            // set_ref(page, 1);
            free->nr_free--;

            size_t size = 1 << current_order;
            while (current_order > level)
            {
#ifdef BUDDY_DEBUG
                kernel_printf("\twhile-page: %x\n", page);
                kernel_getchar();
#endif // BUDDY_DEBUG
                free--;
                current_order--;
                size >>= 1;
                page_t *buddy_page = page + size;
#ifdef BUDDY_DEBUG
                kernel_printf("\tbuddy page: %x\n", page);
                kernel_getchar();
#endif // BUDDY_DEBUG
                list_add(&buddy_page->list, &free->free_head);
                free->nr_free++;
                set_bplevel(buddy_page, current_order);
                set_flag(buddy_page, PAGE_FREE);
            }

            unlock(&buddy.lock);
            return page;
        }
    }

    // if not found, return NULL
    unlock(&buddy.lock);
    return NULL;
}

void *alloc_pages(size_t page_cnt)
{
    if (page_cnt == 0)
    {
        return NULL;
    }

    // find the level of the page_cnt
    size_t level = 0;
    while ((1 << level) < page_cnt)
    {
        level++;
    }

    page_t *page = __alloc_pages(level);

    // return NULL if __alloc_pages() failed.
    if (page == NULL)
    {
        return NULL;
    }
#ifdef BUDDY_DEBUG
    kernel_printf("\tbuddy: alloc_pages: pages=%x, page=%x, returns: %x\n", pages, page, ((page - pages) << PAGE_SHIFT) | KERNEL_ENTRY);
#endif // BUDDY_DEBUG
    return (void *)(((page - pages) << PAGE_SHIFT) | KERNEL_ENTRY);
}

void *buddy_kmalloc(size_t size)
{
    size = UPPER_ALLIGN(size, PAGE_SIZE);
    return (void *)(KERNEL_ENTRY | (unsigned int)alloc_pages(size >> PAGE_SHIFT));
}

void buddy_kfree(void *obj)
{
    page_t *page;
    obj = (void *)((unsigned int)obj & (~KERNEL_ENTRY));
    page = pages + ((unsigned int)obj >> PAGE_SHIFT);
#ifdef BUDDY_DEBUG
    kernel_printf("buddy_kfree: obj: %x, page->bplevel: %d", obj, page->bplevel);
    kernel_getchar();
#endif // ! BUDDY_DEBUG
    free_pages((void *)((unsigned int)obj & PAGE_MASK), page->bplevel);
}

void free_pages(void *addr, size_t bplevel)
{
    __free_pages(pages + ((unsigned int)addr >> PAGE_SHIFT), bplevel);
}

void buddy_info()
{
    kernel_printf("Buddy-system:\n");
    kernel_printf("\tzdtart page-frame number : %x\n", buddy.buddy_start_pfn);
    kernel_printf("\tzrdnd page-frame number : %x\n", buddy.buddy_end_pfn);
    for (size_t index = 0; index <= MAX_BUDDY_ORDER; index++)
    {
        kernel_printf("\t(%x)# : %x frees\n", index, buddy.freelist[index].nr_free);
    }
}
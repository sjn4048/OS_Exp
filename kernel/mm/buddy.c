#include <arch.h>
#include <driver/vga.h>
#include <zjunix/bootmm.h>
#include <zjunix/buddy.h>
#include <zjunix/list.h>
#include <zjunix/lock.h>
#include <zjunix/utils.h>

// initialize all memory with page struct
void init_pages(unsigned int start_pfn, unsigned int end_pfn)
{
    for (int i = start_pfn; i < end_pfn; i++)
    {
        clean_flag(pages + i, -1);
        set_flag(pages + i, _PAGE_RESERVED);
        (pages + i)->reference = 1;
        (pages + i)->virtual = (void *)(-1);
        (pages + i)->bplevel = (-1);
        (pages + i)->slabp = 0;
        // initially, the free space is the whole page
        INIT_LIST_HEAD(&(pages[i].list));
    }
}

void init_buddy()
{
    pages = (struct page*)bootmm_alloc(sizeof(struct page) * bmm.max_pfn, _MM_KERNEL, 1 << PAGE_SHIFT);
    if (!pages)
    {
        // if bootmm fails to provide memory
        kernel_printf("\nERROR : bootmm_alloc failed!\nInit buddy system failed!\n");
        while (1)
            ;
    }

    init_pages(0, bmm.max_pfn);

    int kernel_end_page = 0;
    for (int i = 0; i < bmm.info_cnt; i++)
    {
        if (bmm.info[i].end > kernel_end_page)
            kernel_end_page = bmm.info[i].end;
    }
    kernel_end_page >>= PAGE_SHIFT;

    // the pages that bootmm using cannot be merged into buddy_sys
    buddy.buddy_start_pfn = (kernel_end_page + (1 << MAX_BUDDY_ORDER) - 1) &
                            ~((1 << MAX_BUDDY_ORDER) - 1);
    buddy.buddy_end_pfn = bmm.max_pfn & ~((1 << MAX_BUDDY_ORDER) - 1); // remain 2 pages for I/O

    // init freelists of all bplevels
    for (int i = 0; i <= MAX_BUDDY_ORDER; i++)
    {
        buddy.freelist[i].nr_free = 0;
        INIT_LIST_HEAD(&(buddy.freelist[i].free_head));
    }
    buddy.start_page = pages + buddy.buddy_start_pfn;
    init_lock(&(buddy.lock));

    for (int i = buddy.buddy_start_pfn; i < buddy.buddy_end_pfn; i++)
    {
        __free_pages(pages + i, 0);
    }
}

void __free_pages(struct page *page, unsigned int level)
{
    /* page_idx -> the current page
     * bgroup_idx -> the buddy group that current page is in
     */
    unsigned int page_idx, bgroup_idx;
    unsigned int combined_idx, tmp;
    struct page *bgroup_page;

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
            // kernel_printf("%x %x\n", bgroup_page->bplevel, bplevel);

            break;
        }
        list_del_init(&bgroup_page->list);
        --buddy.freelist[level].nr_free;
        set_bplevel(bgroup_page, -1);
        combined_idx = bgroup_idx & page_idx;
        page += (combined_idx - page_idx);
        page_idx = combined_idx;
        level++;
    }
    set_bplevel(page, level);
    list_add(&(page->list), &(buddy.freelist[level].free_head));
    buddy.freelist[level].nr_free++;
#ifdef BUDDY_DEBUG
    kernel_printf("v%x__addto__%x\n", &(pbpage->list), &(buddy.freelist[bplevel].free_head));
#endif // BUDDY_DEBUG
    unlock(&buddy.lock);
}

struct page *__alloc_pages(unsigned int level)
{
    lockup(&buddy.lock);

    for (int current_order = level; current_order <= MAX_BUDDY_ORDER; current_order++)
    {
        struct freelist *free = buddy.freelist + current_order;
        if (!list_empty(&(free->free_head)))
        {
            struct page *page = container_of(free->free_head.next, struct page, list);
            list_del_init(&(page->list));
            set_bplevel(page, level);
            set_flag(page, _PAGE_ALLOCED);
            // set_ref(page, 1);
            free->nr_free--;

            unsigned int size = 1 << current_order;
            while (current_order > level)
            {
                free--;
                current_order--;
                size >>= 1;
                struct page *buddy_page = page + size;
                list_add(&(buddy_page->list), &(free->free_head));
                free->nr_free++;
                set_bplevel(buddy_page, current_order);
            }

            unlock(&buddy.lock);
            return page;
        }
    }

    // if not found, return NULL
    unlock(&buddy.lock);
    return 0;
}

void *alloc_pages(unsigned int level)
{
    struct page *page = __alloc_pages(level);

    if (!page)
        return 0;

    return (void *)((page - pages) << PAGE_SHIFT | KERNEL_ENTRY);
}

void free_pages(void *addr, unsigned int bplevel)
{
    __free_pages(pages + ((unsigned int)addr >> PAGE_SHIFT), bplevel);
}

void buddy_info()
{
    kernel_printf("Buddy-system :\n");
    kernel_printf("\tstart page-frame number : %x\n", buddy.buddy_start_pfn);
    kernel_printf("\tend page-frame number : %x\n", buddy.buddy_end_pfn);
    for (int index = 0; index <= MAX_BUDDY_ORDER; index++)
    {
        kernel_printf("\t(%x)# : %x frees\n", index, buddy.freelist[index].nr_free);
    }
}
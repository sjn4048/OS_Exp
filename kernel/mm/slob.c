#include <arch.h>
#include <driver/vga.h>
#include <zjunix/slob.h>
#include <zjunix/utils.h>
#include <zjunix/buddy.h>

struct slob_block
{
    int units;
    struct slob_block *next;
};
typedef struct slob_block slob_t;

#define SLOB_BREAK1 256
#define SLOB_BREAK2 1024

#define SLOB_UNIT sizeof(slob_t)
#define SLOB_UNITS(size) (((size) + (SLOB_UNIT)-1) / (SLOB_UNIT))

typedef struct bigblock
{
    int order;
    void *pages;
    struct bigblock *next;
} bigblock_t;

static bigblock_t *bigblocks;

static slob_t free_slob_small = {.next = &free_slob_small, .units = 1};
static slob_t free_slob_medium = {.next = &free_slob_medium, .units = 1};
static slob_t free_slob_large = {.next = &free_slob_large, .units = 1};

static slob_t *slobfree = &free_slob_small;


static void slob_free(void *block, int size)
{
    slob_t *cur, *b = (slob_t *)block;

    if (!block)
    {
        return;
    }

    if (size)
    {
        b->units = SLOB_UNITS(size);
    }

    /* Find reinsertion point */
    for (cur = slobfree; !(b > cur && b < cur->next); cur = cur->next)
    {
        if (cur >= cur->next && (b > cur || b < cur->next))
        {
            break;
        }
    }
    if (b + b->units == cur->next)
    {
        b->units += cur->next->units;
        b->next = cur->next->next;
    }
    else
    {
        b->next = cur->next;
    }

    if (cur + cur->units == b)
    {
        cur->units += b->units;
        cur->next = b->next;
    }
    else
    {
        cur->next = b;
    }

    slobfree = cur;
}

static void *slob_alloc(unsigned int size)
{
    slob_t *prev, *cur, *aligned = 0;
    int delta = 0, units = SLOB_UNITS(size);

    if (size < SLOB_BREAK1)
    {
        slobfree = &free_slob_small;
    }
    else if (size < SLOB_BREAK2)
    {
        slobfree = &free_slob_medium;
    }
    else
    {
        slobfree = &free_slob_large;
    }

    for (cur = prev->next;; prev = cur, cur = cur->next)
    {
        if (cur->units >= units + delta)
        {
            // if room is enough
            if (delta)
            {
                // need to fragment head to align?
                aligned->units = cur->units - delta;
                aligned->next = cur->next;
                cur->next = aligned;
                cur->units = delta;
                prev = cur;
                cur = aligned;
            }

            if (cur->units == units)
            {
                // if exact fit, unlink
                prev->next = cur->next;
            }
            else
            {
                // make fragment
                prev->next = cur + units;
                prev->next->units = cur->units - units;
                prev->next->next = cur->next;
                cur->units = units;
            }

            slobfree = prev;
            return cur;
        }
        if (cur == slobfree)
        {
            // trying to shrink arena
            if (size == PAGE_SIZE)
            {
                return 0;
            }
            cur = (slob_t *)alloc_pages(0);
            if (!cur)
            {
                return 0;
            }

            slob_free(cur, PAGE_SIZE);
            cur = slobfree;
        }
    }
}

static int find_order(int size)
{
	int order = 0;
	for ( ; size > 4096 ; size >>=1)
		order++;
	return order;
}

void *slob_kmalloc(unsigned int size)
{
    slob_t *m;
    bigblock_t *bb;

    // if the size is not bigger than the page size
    if (size < PAGE_SIZE - SLOB_UNIT)
    {
        m = slob_alloc(size + SLOB_UNIT);
        return m ? (void *)(m + 1) : 0;
    }

    // else call buddy to solve this
    bb = slob_alloc(sizeof(bigblock_t));
	if (!bb) 
    {
		return 0;
    }

    // calculate the order, which is temporarily not used.
    size += (1 << PAGE_SHIFT) - 1;
    size &= ~((1 << PAGE_SHIFT) - 1);
    bb->order = find_order(size);
	bb->pages = alloc_pages(size >> PAGE_SHIFT);

    if (bb->pages)
    {
        bb->next = bigblocks;
        bigblocks = bb;
        return (void *)(KERNEL_ENTRY | (unsigned int)bb->pages);
    };

    slob_free(bb, sizeof(bigblock_t));
    return 0;
}

void *slob_kzalloc(unsigned int size)
{
    void *ret = slob_kmalloc(size);
    if (ret)
        kernel_memset(ret, 0, size);
    return ret;
}

void slob_kfree(const void *block)
{
	bigblock_t *bb, **last = &bigblocks;

    if (!block)
    {
        return;
    }

    if (!((unsigned int)block & (PAGE_SIZE - 1)))
    {
        /* might be on the big block list */
        for (bb = bigblocks; bb; last = &bb->next, bb = bb->next)
        {
            if (bb->pages == block)
            {
                *last = bb->next;
                free_pages((void*)block, bb->order);
                slob_free(bb, sizeof(bigblock_t));
                return;
            }
        }
    }

    slob_free((slob_t *)block - 1, 0);
    return;
}

unsigned int ksize(const void *block)
{
	bigblock_t *bb;

	if (!block)
		return 0;

	if (!((unsigned int)block & (PAGE_SIZE-1))) {
		for (bb = bigblocks; bb; bb = bb->next)
        {
			if (bb->pages == block) {
				return PAGE_SIZE << bb->order;
			}
        }
	}

	return ((slob_t *)block - 1)->units * SLOB_UNIT;
}

// struct kmem_cache {
// 	unsigned int size, align;
// 	const char *name;
// 	void (*ctor)(void *, kmem_cache_t *, unsigned long);
// 	void (*dtor)(void *, kmem_cache_t *, unsigned long);
// };


// #include <arch.h>
// #include <driver/vga.h>
// #include <zjunix/slob.h>
// #include <zjunix/utils.h>
// #include <zjunix/slab.h>

// /* as there is literally no concept as int16, here we use both int as the
//  * slobidx_t.
//  */
// #if PAGE_SIZE <= (32767 * 2)
// typedef int slobidx_t;
// #else
// typedef int slobidx_t;
// #endif

// /*
//  * slob_block has a field 'units', which indicates size of block if +ve,
//  * or offset of next block if -ve (in SLOB_UNITs).
//  *
//  * Free blocks of size 1 unit simply contain the offset of the next block.
//  * Those with larger size contain their size in the first SLOB_UNIT of
//  * memory, and the offset of the next free block in the second SLOB_UNIT.
//  */
// typedef struct slob_block
// {
//     int units;
// } slob_t;

// #define SLOB_BREAK1 256
// #define SLOB_BREAK2 1024

// /*
//  * All partially free slob pages go on these lists.
//  */
// static LIST_HEAD(free_slob_small);
// static LIST_HEAD(free_slob_medium);
// static LIST_HEAD(free_slob_large);

// /*
//  * slob_page_free: true for pages on free_slob_pages list.
//  */
// static inline int slob_page_free(struct page *sp)
// {
//     // slobp == 0 means it's free
//     return sp->slobp == PAGE_FREE;
// }

// /*
//  * set_slob_page_free: add page into free_list and set the slobp flag to true.
//  */
// static void set_slob_page_free(struct page *sp, struct list_head *list)
// {
//     // TODO
// 	list_add(&sp->??, list);
// 	sp->slobp = PAGE_FREE;
// }

// static inline void clear_slob_page_free(struct page *sp)
// {
//     // TODO
// }

// #define SLOB_UNIT sizeof(slob_t)
// #define SLOB_UNITS(size) (((size) + (SLOB_UNIT) - 1) / (SLOB_UNIT))

// /*
//  * Encode the given size and next info into a free slob block s.
//  */
// static void set_slob(slob_t *s, slobidx_t size, slob_t *next)
// {
//     slob_t *base = (slob_t*)((unsigned int)s >> PAGE_SHIFT);
//     slobidx_t offset = next - base;

//     if (size > 1)
//     {
//         s[0].units = size;
//         s[1].units = offset;
//     }
//     else
//     {
//         s[0].units = -offset;
//     }
// }

// /*
//  * Return the size of a slob block.
//  */
// static slobidx_t slob_units(slob_t *s)
// {
// 	if (s->units > 0)
// 		return s->units;
// 	return 1;
// }

// /*
//  * Return the next free slob block pointer after this one.
//  */
// static slob_t *slob_next(slob_t *s)
// {
// 	slob_t *base = (slob_t *)((unsigned int)s >> PAGE_SHIFT);
// 	slobidx_t next;

// 	if (s[0].units < 0)
//     {
// 		next = -s[0].units;
//     }
//     else
// 	{
//     	next = s[1].units;
//     }
//     return base + next;
// }

// /*
//  * Returns true if s is the last free block in its page.
//  */
// static int slob_last(slob_t *s)
// {
// 	return !((unsigned int)slob_next(s) >> PAGE_SHIFT);
// }

// // returns the address of new page
// static void *slob_new_pages()
// {
// 	void *page = __alloc_pages(0);

// 	if (!page)
// 		return NULL;

// 	return (page << PAGE_SHIFT) | KERNEL_ENTRY;
// }

// static void slob_free_pages(void *b, int order)
// {
// 	// TODO
// }

// /*
//  * Allocate a slob block within a given slob_page sp.
//  */
// static void *slob_page_alloc(struct page *sp, size_t size, int align)
// {

// }

// /*
//  * slob_alloc: entry point into the slob allocator.
//  */
// static void *slob_alloc(unsigned int size)
// {
//     struct slob_t *sp;
// 	struct list_head *prev;
// 	struct list_head *slobfree;
// 	slob_t *b = NULL;
// 	unsigned long flags;

//     // choose which slob to allocate
//     if (size < SLOB_BREAK1)
//     {
//         slobfree = &free_slob_small;
//     }
//     else if (size < SLOB_BREAK2)
//     {
//         slobfree = &free_slob_medium;
//     }
//     else
//     {
//         slobfree = free_slob_large;
//     }

//     list_for_each_safe(sp, slobfree, list)
//     {
//         // not enough room on this page
//         if (sp->units < SLOB_UNITS(size))
//         {
//             continue;
//         }

//         prev = sp->list
//     }
// }

// /*
//  * slob_free: entry point into the slob allocator.
//  */
// static void slob_free(void *block, int size)
// {

// }
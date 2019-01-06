#include <arch.h>
#include <driver/vga.h>
#include <zjunix/slob.h>
#include <zjunix/utils.h>
#include <zjunix/buddy.h>
#include <zjunix/slab.h>

struct slob_block
{
    int units;
    struct slob_block *next;
};
typedef struct slob_block slob_t;

#ifdef SLOB_SINGLE
static slob_t arena = {.next = &arena, .units = 1};
static slob_t *slobfree = &arena;
#else
#define SLOB_BREAK1 256
#define SLOB_BREAK2 1024
static slob_t free_slob_small = {.next = &free_slob_small, .units = 1};
static slob_t free_slob_medium = {.next = &free_slob_medium, .units = 1};
static slob_t free_slob_large = {.next = &free_slob_large, .units = 1};

static slob_t *small_cur = &free_slob_small;
static slob_t *medium_cur = &free_slob_medium;
static slob_t *large_cur = &free_slob_large;

static slob_t *slobfree = &free_slob_small;
#endif

#define SLOB_UNIT sizeof(slob_t)
#define SLOB_UNITS(size) (((size) + (SLOB_UNIT)-1) / (SLOB_UNIT))

typedef struct bigblock
{
    int order;
    void *pages;
    struct bigblock *next;
} bigblock_t;

static bigblock_t *bigblocks;

extern void init_slob()
{
    // TODO
}

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
#ifdef SLOB_DEBUG
    kernel_printf("enter slob_alloc. Size: %d\n", size);
    kernel_getchar();
#endif // SLOB_DEBUG
    slob_t *prev, *cur, *aligned = 0;
    int delta = 0, units = SLOB_UNITS(size);
#ifdef SLOB_SINGLE
    prev = slobfree;
#else
    // if use multiple slobs
    if (size < SLOB_BREAK1)
    {
#ifdef SLOB_DEBUG
        kernel_printf("Use small slob to handle.\n");
        kernel_getchar();
#endif // SLOB_DEBUG
        slobfree = &free_slob_small;
    }
    else if (size < SLOB_BREAK2)
    {
#ifdef SLOB_DEBUG
        kernel_printf("Use medium slob to handle.\n");
        kernel_getchar();
#endif // SLOB_DEBUG
        slobfree = &free_slob_medium;
    }
    else
    {
#ifdef SLOB_DEBUG
        kernel_printf("Use large slob to handle.\n");
        kernel_getchar();
#endif // SLOB_DEBUG
        slobfree = &free_slob_large;
    }
#endif

    for (cur = prev->next;; prev = cur, cur = cur->next)
    {
#ifdef SLOB_DEBUG
        kernel_printf("Cur: %x\tPrev: %x\t\n", cur, prev);
        kernel_printf("cur->units: %d\tunits: %d\tdelta: %d\n", cur->units, units, delta);
        kernel_getchar();
#endif // SLOB_DEBUG
        if (cur->units >= units + delta)
        {
#ifdef SLOB_DEBUG
            kernel_printf("room is enough.\n");
            kernel_getchar();
#endif // SLOB_DEBUG \
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
#ifdef SLOB_DEBUG
                kernel_printf("exact fit.\n");
                kernel_getchar();
#endif // SLOB_DEBUG \
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
#ifdef SLOB_DEBUG
                kernel_printf("After fragment.\n");
                kernel_printf("prev->next: %x, prev->next->units: %d, prev->next->next: %x, cur->units: %d", prev->next, prev->next->units, prev->next->next, cur->units);
                kernel_getchar();
#endif // SLOB_DEBUG
            }

            slobfree = prev;
#ifdef SLOB_DEBUG
            kernel_printf("slob_alloc returns %x.\n", cur);
            kernel_getchar();
#endif // SLOB_DEBUG
            return cur;
        }
        if (cur == slobfree)
        {
            // trying to shrink arena
            if (size == PAGE_SIZE)
            {
#ifdef SLOB_DEBUG
                kernel_printf("tring to shrink arena. returns 0 as size == PAGE_SIZE.\n");
                kernel_getchar();
#endif // SLOB_DEBUG
                return 0;
            }
            cur = (slob_t *)alloc_pages(1);
#ifdef SLOB_DEBUG
            kernel_printf("alloc new page %x for cur. Page no: %d\n", cur, ((unsigned int)cur - KERNEL_ENTRY) >> PAGE_SHIFT);
            kernel_getchar();
#endif // SLOB_DEBUG
            if (!cur)
            {
                return 0;
            }
#ifdef SLOB_DEBUG
            kernel_printf("call slob_free for cur.\n");
            kernel_getchar();
#endif // SLOB_DEBUG
            slob_free(cur, PAGE_SIZE);
            cur = slobfree;
        }
    }
}

static int find_order(int size)
{
    int order = 0;
    for (; size > 4096; size >>= 1)
        order++;
    return order;
}

void *slob_kmalloc(unsigned int size)
{
    slob_t *m;
    bigblock_t *bb;
#ifdef SLOB_DEBUG
    kernel_printf("kmalloc %d memory in slob.\n", size, PAGE_SIZE, SLOB_UNIT, PAGE_SIZE - SLOB_UNIT);
    kernel_getchar();
#endif // SLOB_DEBUG

    // if the size is not bigger than the page size
    if (size < PAGE_SIZE - SLOB_UNIT)
    {
#ifdef SLOB_DEBUG
        kernel_printf("Handle malloc in slob.\n");
        kernel_getchar();
#endif // SLOB_DEBUG
        m = slob_alloc(size + SLOB_UNIT);
        return m ? (void *)(m + 1) : 0;
    }

    // else call buddy to solve this
#ifdef SLOB_DEBUG
    kernel_printf("Handle malloc in buddy instead.\n");
    kernel_getchar();
#endif // SLOB_DEBUG
    bb = slob_alloc(sizeof(bigblock_t));
    if (!bb)
    {
        return 0;
    }

    // calculate the order, which is temporarily not used.
    size = UPPER_ALLIGN(size, PAGE_SIZE);
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
                free_pages((void *)block, bb->order);
                slob_free(bb, sizeof(bigblock_t));
                return;
            }
        }
    }

    slob_free((slob_t *)block - 1, 0);
    return;
}

unsigned int slob_ksize(const void *block)
{
    bigblock_t *bb;

    if (!block)
        return 0;

    if (!((unsigned int)block & (PAGE_SIZE - 1)))
    {
        for (bb = bigblocks; bb; bb = bb->next)
        {
            if (bb->pages == block)
            {
                return PAGE_SIZE << bb->order;
            }
        }
    }

    return ((slob_t *)block - 1)->units * SLOB_UNIT;
}

struct kmem_cache_s {
	unsigned int size, align;
	const char *name;
	void (*ctor)();
	void (*dtor)();
};

kmem_cache *kmem_cache_create(char *name, unsigned int size, unsigned int align,
	void (*ctor)(),
	void (*dtor)())
{
	kmem_cache *c;

	c = slob_alloc(sizeof(kmem_cache));

	if (c) {
		c->name = name;
		c->size = size;
		// c->ctor = ctor;
		// c->dtor = dtor;
		/* ignore alignment */
		// c->align = 0;
		// if (c->align < align)
		// c->align = align;
	}

	return c;
}

int kmem_cache_destroy(kmem_cache *c)
{
	slob_free(c, sizeof(kmem_cache));
	return 0;
}

void *kmem_cache_alloc(kmem_cache *c)
{
	void *b;

	if (c->size < PAGE_SIZE)
		b = slob_alloc(c->size);
	else
		b = (void *)__alloc_pages(find_order(c->size));

	return b;
}

void kmem_cache_free(kmem_cache *c, void *b)
{
	if (c->size < PAGE_SIZE)
		slob_free(b, c->size);
	else
		free_pages(b, find_order(c->size));
}

unsigned int kmem_cache_size(kmem_cache *c)
{
	return c->size;
}

const char *kmem_cache_name(kmem_cache *c)
{
	return c->name;
}

void kmem_cache_init(void)
{
	void *p = slob_alloc(PAGE_SIZE);

	if (p)
		free_pages(p, 0);
}
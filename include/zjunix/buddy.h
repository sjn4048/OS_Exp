#ifndef _ZJUNIX_BUDDY_H
#define _ZJUNIX_BUDDY_H

#include <zjunix/list.h>
#include <zjunix/lock.h>

/*
 * order means the size of the set of pages, e.g. order = 1 -> 2^1
 * pages(consequent) are free In current system, we allow the max order to be
 * 4(2^4 consequent free pages)
 */
#define MAX_BUDDY_ORDER 4

typedef struct freelist {
    size_t nr_free;
    struct list_head free_head;
} freelist_t;

typedef struct buddy_sys {
    off_t buddy_start_pfn;
    off_t buddy_end_pfn;
    page_t *start_page;
    struct lock_t lock;
    freelist_t freelist[MAX_BUDDY_ORDER + 1];
} buddy_t;

struct buddy_sys buddy;

#define _is_same_bpgroup(page, bage) (((*(page)).bplevel == (*(bage)).bplevel))
#define _is_same_bplevel(page, lval) ((*(page)).bplevel == (lval))
#define set_bplevel(page, lval) ((*(page)).bplevel = (lval))
#define set_flag(page, val) ((*(page)).flag = (val))
#define clean_flag(page, val) ((*(page)).flag = 0)
#define has_flag(page, val) ((*(page)).flag & val)
#define set_ref(page, val) ((*(page)).reference = (val))
#define inc_ref(page, val) ((*(page)).reference += (val))
#define dec_ref(page, val) ((*(page)).reference -= (val))

extern void __free_pages(struct page *page, unsigned int order);
extern struct page *__alloc_pages(unsigned int order);

extern void free_pages(void *addr, unsigned int order);

extern void *alloc_pages(unsigned int order);

extern void *buddy_kmalloc(unsigned int size);

extern void buddy_kfree(void* obj);

extern void init_buddy();

extern void buddy_info();

#endif

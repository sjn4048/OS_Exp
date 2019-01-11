#ifndef _MEMORY_H
#define _MEMORY_H

#include <zjunix/list.h>

#ifndef NULL
#define NULL 0
#endif // !NULL

#pragma region debug

//#define MEM_CHOOSE_DEBUG
#ifdef MEM_CHOOSE_DEBUG
#include <driver/ps2.h>
#endif

//#define BOOTMM_DEBUG
#ifdef BOOTMM_DEBUG
#include <driver/ps2.h>
#endif

//#define BUDDY_DEBUG
#ifdef BUDDY_DEBUG
#include <driver/ps2.h>
#endif //BUDDY_DEBUG

//#define SLOB_DEBUG
#ifdef SLOB_DEBUG
#include <driver/ps2.h>
#endif // DEBUG

//#define MEMORY_DEBUG
#ifdef MEMORY_DEBUG
#include <driver/ps2.h>
#endif

//#define MEMORY_TEST_DEBUG
#ifdef MEMORY_TEST_DEBUG
#include <driver/ps2.h>
#endif

//#define SLAB_DEBUG
#ifdef SLAB_DEBUG
#include <driver/ps2.h>
#endif

#define VMA_DEBUG
#ifdef VMA_DEBUG
#include <driver/ps2.h>
#endif

#define TLB_DEBUG
#ifdef TLB_DEBUG
#include <driver/ps2.h>
#endif // TLB_DEBUG

#pragma endregion

#define _PAGE_RESERVED (1 << 31)
#define _PAGE_ALLOCED (1 << 30)
#define _PAGE_SLAB (1 << 29)
#define BYTES_PER_WORD sizeof(void *)

#define PAGE_SHIFT 12
#ifndef PAGE_SIZE
#define PAGE_SIZE (1 << (PAGE_SHIFT))
#endif // !PAGE_SIZE
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define INDEX_MASK 0x3ff //低10佝
#define PGD_SHIFT 22
#define PGD_SIZE (1 << PAGE_SHIFT)
#define PGD_MASK (~((1 << PGD_SHIFT) - 1))
#define PGD_SPAN 1024

/*
 * struct buddy page is one info-set for the buddy group of pages
 */

typedef struct page
{
    unsigned int flag;      // the declaration of the usage of this page
    unsigned int reference; //
    struct list_head list;  // double-way list
    void *virtual;          // default 0x(-1)
    unsigned int bplevel;   /* the order level of the page
                              *
                              * unsigned int sl_objs;
                              * 		represents the number of objects in current
                              * if the page is of _PAGE_SLAB, then bplevel is the sl_objs
                              */
    unsigned int slabp;     /* if the page is used by slab system,
                              * then slabp represents the base-addr of free space
                              */
} page_t;

typedef unsigned int size_t;
typedef unsigned int off_t;
typedef char boolean_t;
#define True 1
#define False 0

typedef unsigned int pgd_t;
typedef unsigned long addr_t;

//#include <zjunix/slub.h>

enum __Allocator
{
    SLAB,
    SLUB,
    SLOB,
    BUDDY,
    BOOTMM
} allocator;

#ifdef __CONFIG_SLUB
allocator = SLUB;
#endif

#ifdef __CONFIG_BUDDY
allocator = BUDDY;
#endif

#ifdef __CONFIG_SLOB
allocator = SLOB;
#endif

#ifdef __CONFIG_SLAB
allocator = SLAB;
#endif

static char *all_to_str[] = {"slab", "slub", "slob", "buddy"};

page_t *pages;

void *kmalloc(unsigned int size);
void kfree(void *obj);
void test_malloc_accuracy();

void mem_test();
void test_malloc_speed();
void test_reg_base();
char *mem_choose();

#endif
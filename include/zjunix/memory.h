#ifndef _MEMORY_H
#define _MEMORY_H

#include <zjunix/slab.h>
#include <zjunix/slob.h>
#include <zjunix/buddy.h>
#include <driver/vga.h>

#ifndef NULL
#define NULL 0
#endif // !NULL

//#define MEMORY_DEBUG
#ifdef MEMORY_DEBUG
#include <driver/ps2.h>
#endif

//#include <zjunix/slub.h>

enum __Allocator
{
    SLAB,
    SLUB,
    SLOB,
    BUDDY
} allocator;

static char *all_to_str[] = { "slab", "slub", "slob", "buddy" };

void *kmalloc(unsigned int size);
void kfree(void *obj);

void test_malloc();

#endif
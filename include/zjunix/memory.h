#ifndef _MEMORY_H
#define _MEMORY_H

#include <zjunix/slab.h>
#include <zjunix/slob.h>

#ifndef NULL
#define NULL 0
#endif // !NULL

//#include <zjunix/slub.h>

enum __Allocator
{
    SLAB,
    SLUB,
    SLOB,
    BUDDY
} allocator;

void *kmalloc(unsigned int size);
void kfree(void *obj);

#endif
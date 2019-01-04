#include <zjunix/memory.h>

void *kmalloc(unsigned int size)
{
    switch (allocator)
    {
    case SLAB:
    case SLUB:
    case SLOB:
    case BUDDY:
    default:
        break;
    }
}

void kfree(void *obj)
{
    switch (allocator)
    {
    case SLAB:
    case SLUB:
    case SLOB:
    case BUDDY:
    default:
        break;
    }
}
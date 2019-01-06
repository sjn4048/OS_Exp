#include <zjunix/memory.h>

void *kmalloc(unsigned int size)
{
    // TODO
    allocator = SLOB;

    switch (allocator)
    {
    case SLAB:
        slab_kmalloc(size);
        break;
    case SLUB:
    case SLOB:
        slob_kmalloc(size);
        break;
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
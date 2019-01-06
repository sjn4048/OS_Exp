#include <zjunix/memory.h>
#include <zjunix/time.h>

void *kmalloc(unsigned int size)
{
    // TODO
    allocator = SLOB;
    void *res;
#ifdef MEMORY_DEBUG
    kernel_printf("Kmalloc: Size:%d, Allocator: %s\t", size, all_to_str[allocator]);
    kernel_getchar();
#endif // SLOB_DEBUG

    switch (allocator)
    {
    case SLAB:
        res = slab_kmalloc(size);
        break;
    case SLUB:
    case SLOB:
        res = slob_kmalloc(size);
        break;
    case BUDDY:
        res = buddy_kmalloc(size);
        break;
    default:
        break;
    }
#ifdef MEMORY_DEBUG
    kernel_printf("Returns: %x\n", res);
    kernel_getchar();
#endif // SLOB_DEBUG
    return res;
}

void kfree(void *obj)
{
#ifdef MEMORY_DEBUG
    kernel_printf("Kfree: obj:%x, Allocator: %s\t", obj, all_to_str[allocator]);
    kernel_getchar();
#endif // SLOB_DEBUG
    switch (allocator)
    {
    case SLAB:
        slab_kfree(obj);
        break;
    case SLUB:
    case SLOB:
        slob_kfree(obj);
        break;
    case BUDDY:
        buddy_kfree(obj);
        break;
    default:
        break;
    }
#ifdef MEMORY_DEBUG
    kernel_printf("Done.\n");
    kernel_getchar();
#endif // SLOB_DEBUG
}

#pragma GCC push_options
#pragma GCC optimize("O0")

#define RETRY_CNT 1000
#define RETRY_CNT2 5000
#define RETRY_SIZE 2000
void test_malloc()
{
    void *a;
    //TODO:known bug
    // a = kmalloc(10);
    // kfree(a);
    // a = kmalloc(100);
    // kfree(a);
    // a = kmalloc(256);
    // kfree(a);
    // a = kmalloc(512);
    // kfree(a);
    // a = kmalloc(1024);
    // kfree(a);
    // a = kmalloc(2048);
    // kfree(a);
    // a = kmalloc(4096);
    // kfree(a);
    // a = kmalloc(10000);
    // kfree(a);

    void *b[RETRY_CNT];
    char time_str_buf[9];
    get_time(time_str_buf, sizeof(time_str_buf) / sizeof(char));
    kernel_printf("%s", time_str_buf);
    for (int j = 0; j < RETRY_CNT2; j++)
    {
        for (int i = 0; i < RETRY_CNT; i++)
        {
            b[i] = (char *)kmalloc(RETRY_SIZE);
        }
        for (int i = 0; i < RETRY_CNT; i++)
        {
            kfree(b[i]);
        }
    }
    get_time(time_str_buf, sizeof(time_str_buf) / sizeof(char));
    kernel_printf("%s", time_str_buf);
}
#pragma GCC pop_options
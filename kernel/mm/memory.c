#include <zjunix/memory.h>
#include <zjunix/time.h>
#include <zjunix/slab.h>
#include <zjunix/slob.h>
#include <zjunix/buddy.h>
#include <driver/vga.h>
#include <zjunix/log.h>
#include <zjunix/bootmm.h>
#include <zjunix/fs/fat.h>

char *mem_choose()
{
    char *filename = "allocator";

    FILE file;
    int result = fs_open(&file, filename);

    if (result == 0 && get_entry_filesize(file.entry.data) < 15)
    {
        unsigned char buffer[15];
        fs_read(&file, buffer, 15);

        if (kernel_strcmp(buffer, "slob"))
        {
            allocator = SLOB;
        }
        else if (kernel_strcmp(buffer, "slab"))
        {
            allocator = SLAB;
        }
        else if (kernel_strcmp(buffer, "slub"))
        {
            //allocator = SLUB;
        }
        else if (kernel_strcmp(buffer, "buddy"))
        {
            allocator = BUDDY;
        }
        else if (kernel_strcmp(buffer, "bootmm"))
        {
            allocator = BOOTMM;
        }
    }
    else
    {
        // default allocator: SLOB
        allocator = SLOB;
    }
    return all_to_str[allocator];
}

// Top-most and only memory allocation entry
void *kmalloc(size_t size)
{
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
    case BOOTMM:
        res = bootmm_kmalloc(size);
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

// Top-most and only memory free entry
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
    case BOOTMM:
        bootmm_kfree(obj);
        break;
    default:
        break;
    }
#ifdef MEMORY_DEBUG
    kernel_printf("Done.\n");
    kernel_getchar();
#endif // SLOB_DEBUG
}

void mem_test()
{
    test_malloc_accuracy();
}

#pragma GCC push_options
#pragma GCC optimize("O0")

#define ACCURACY_RETRY_IN 300
#define ACCURACY_EXPAND_RATE 1

void test_malloc_accuracy()
{
    int *a[ACCURACY_RETRY_IN];

#ifdef MEMORY_TEST_DEBUG
    kernel_printf("Phase 1...\n");
#endif // DEBUG

    // initialize all the elements in a[N];
    for (size_t i = 1; i < ACCURACY_RETRY_IN; i++)
    {
        a[i] = (int *)kmalloc(sizeof(int) * i * ACCURACY_EXPAND_RATE);
#ifdef MEMORY_TEST_DEBUG
        kernel_printf("a[%d]: size: %d, 1st: %x ", i, sizeof(int) * i * ACCURACY_EXPAND_RATE, (unsigned int)a[i]);
#endif // DEBUG
        kfree(a[i]);
        a[i] = (int *)kmalloc(sizeof(int) * i * ACCURACY_EXPAND_RATE);
#ifdef MEMORY_TEST_DEBUG
        kernel_printf("2nd: %x\n", (unsigned int)a[i]);
#endif // DEBUG
        *a[i] = i;
#ifdef MEMORY_TEST_DEBUG
        if (i >= 1)
        {
            for (size_t k = 1; k <= i; k++)
            {
                kernel_printf("%d ", *a[k]);
            }
            kernel_printf("\n");
            kernel_getchar();
        }
#endif // DEBUG
    }
#ifdef MEMORY_TEST_DEBUG
    kernel_printf("Phase 2...\n");
#endif // DEBUG
    for (size_t i = 1; i < ACCURACY_RETRY_IN; i++)
    {
        if (*a[i] != i)
        {
            log(LOG_FAIL, "Fail to validate memory allocation: error at %d", i);
        }
        // free it after test.
        kfree(a[i]);
#ifdef MEMORY_TEST_DEBUG
        kernel_printf("%d\t", i);
#endif // DEBUG
    }
}

#pragma GCC pop_options

#pragma GCC push_options
#pragma GCC optimize("O0")

#define RETRY_CNT 1000
#define RETRY_CNT2 5000
#define RETRY_SIZE 2000
void test_malloc_speed()
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
    for (size_t j = 0; j < RETRY_CNT2; j++)
    {
        for (size_t i = 0; i < RETRY_CNT; i++)
        {
            b[i] = (char *)kmalloc(RETRY_SIZE);
        }
        for (size_t i = 0; i < RETRY_CNT; i++)
        {
            kfree(b[i]);
        }
    }
    get_time(time_str_buf, sizeof(time_str_buf) / sizeof(char));
    kernel_printf("%s", time_str_buf);
}
#pragma GCC pop_options

#pragma GCC push_options
#pragma GCC optimize("O0")

//#include <driver/ps2.h>
void test_reg_base()
{
    unsigned int test_start = 0x10;
    unsigned int out;

    asm volatile(
        "move $t0, %0\n\t"
        "mtc0 $t0, $7\n\t"
        "nop\n\t"
        : "=r"(test_start));

    asm volatile(
        "nop\n\t"
        "mfc0 $t1, $7\n\t"
        "move %0, $t1\n\t"
        "nop\n\t"
        : "=r"(out));

    kernel_printf("out: %x\n", out);
}

#pragma GCC pop_options
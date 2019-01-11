#include <zjunix/memory.h>
#include <zjunix/time.h>
#include <zjunix/slab.h>
#include <zjunix/slob.h>
#include <zjunix/buddy.h>
#include <driver/vga.h>
#include <zjunix/log.h>
#include <zjunix/bootmm.h>
#include <zjunix/fs/ext2.h>

char* get_allocator_name()
{
    return all_to_str[allocator];
}

/* Choose memory allocator
 * from sd card.
 * 
 * 
 */
char *mem_choose()
{
    char *filename = "/allocator";

    EXT2_FILE file;
    int result = ext2_open(filename, &file);

    if (result == EXT2_SUCCESS && file.inode.info.i_size < 128)
    {
#ifdef MEM_CHOOSE_DEBUG
        kernel_printf("\tFile found.\n");
#endif
        unsigned char buffer[128];
        kernel_memset(buffer, 0, sizeof(buffer));
        unsigned long length;
        ext2_read(&file, buffer, 128, &length);
#ifdef MEM_CHOOSE_DEBUG
        kernel_printf("\tIn file: %s\n", buffer);
#endif
        if (kernel_strcmp(buffer, "slob") == 0)
        {
            allocator = SLOB;
        }
        else if (kernel_strcmp(buffer, "slab") == 0)
        {
            allocator = SLAB;
        }
        else if (kernel_strcmp(buffer, "slub") == 0)
        {
            //allocator = SLUB;
        }
        else if (kernel_strcmp(buffer, "buddy") == 0)
        {
            allocator = BUDDY;
        }
        else if (kernel_strcmp(buffer, "bootmm") == 0)
        {
            allocator = BOOTMM;
        }
        else
        {
            // default allocator: SLOB
            allocator = SLOB;
        }
#ifdef MEM_CHOOSE_DEBUG
        kernel_printf("Allocator: %d\n", allocator);
#endif
    }
    else
    {
#ifdef MEM_CHOOSE_DEBUG
        kernel_printf("File not found.\n");
#endif
        // default allocator: SLOB
        allocator = SLOB;
    }
    return all_to_str[allocator];
}

// Top-most and only memory allocation entry
void *kmalloc(size_t size)
{
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

#pragma GCC push_options
#pragma GCC optimize("O0")

#define ACCURACY_RETRY_SIZE 10
#define ACCURACY_EXPAND_RATE 1

void test_malloc_accuracy()
{
    // test cases where big nodes are allocated.
    // void *b;
    // b = kmalloc(10);
    // kfree(b);
    // b = kmalloc(100);
    // kfree(b);
    // b = kmalloc(256);
    // kfree(b);
    // b = kmalloc(512);
    // kfree(b);
    // b = kmalloc(1024);
    // kfree(b);
    // b = kmalloc(2048);
    // kfree(b);
    // b = kmalloc(4096);
    // kfree(b);
    // b = kmalloc(10000);
    // kfree(b);
 
    int *a[ACCURACY_RETRY_SIZE];

#ifdef MEMORY_TEST_DEBUG
    kernel_printf("Phase 1...\n");
#endif // DEBUG

    // initialize all the elements in a[N];
    for (size_t i = 1; i < ACCURACY_RETRY_SIZE; i++)
    {
        a[i] = (int *)kmalloc(sizeof(int) * i * ACCURACY_EXPAND_RATE);
//#ifdef MEMORY_TEST_DEBUG
        kernel_printf("a[%d]: size: %d, 1st: %x ", i, sizeof(int) * i * ACCURACY_EXPAND_RATE, (unsigned int)a[i]);
//#endif // DEBUG
        kfree(a[i]);
        a[i] = (int *)kmalloc(sizeof(int) * i * ACCURACY_EXPAND_RATE);
//#ifdef MEMORY_TEST_DEBUG
        kernel_printf("2nd: %x\n", (unsigned int)a[i]);
//#endif // DEBUG
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
    for (size_t i = 1; i < ACCURACY_RETRY_SIZE; i++)
    {
        if (*a[i] != i)
        {
            // log(LOG_FAIL, "Fail to validate memory allocation: error at %d, now %d. Addr: %x", i, *a[i], a[i]);
            // kernel_getchar();
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

#define SPEED_ALLOC_CNT 100
#define SPEED_RETRY_TIMES 10000
#define SPEED_SIZE 4
void test_malloc_speed()
{
    void *b[SPEED_ALLOC_CNT];
    char time_str_buf[9];
    get_time(time_str_buf, sizeof(time_str_buf) / sizeof(char));
    kernel_printf("\n--------------\n"
                  "Speed test:\n\tstart: %s\n", time_str_buf);
    for (size_t j = 0; j < SPEED_RETRY_TIMES; j++)
    {
        for (size_t i = 0; i < SPEED_ALLOC_CNT; i++)
        {
            b[i] = kmalloc(SPEED_SIZE);
        }
        for (size_t i = 0; i < SPEED_ALLOC_CNT; i++)
        {
            kfree(b[i]);
        }
    }
    get_time(time_str_buf, sizeof(time_str_buf) / sizeof(char));
    kernel_printf("\tend: %s\n"
                  "--------------\n", time_str_buf);
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

#include <driver/ps2.h>

#pragma GCC push_options
#pragma GCC optimize("O0")
// entry for memory test programs.
void mem_test()
{
    kernel_printf("Memory test v1.0\n"
                  "\tPress 1 for accuracy test\n"
                  "\tPress 2 for speed test\n"
                  "\tPress 3 for unit benchmark.\n"
                  "\tPress 4 for mem_info\n");

    char input = kernel_getchar();
    if (input == '1')
    {
        test_malloc_accuracy();
        log(LOG_OK, "Accuracy test done.");
    }
    else if (input == '2')
    {
        test_malloc_speed();
        log(LOG_OK, "Speed test done.");
    }
    else if (input == '3')
    {
        test_malloc_accuracy();
        test_malloc_speed();
        log(LOG_OK, "Accuracy test done.");
    }
    else if (input == '4')
    {
        print_bootmap();
        buddy_info();
        log(LOG_OK, "Print mem_info done.");
    }
}

#pragma GCC pop_options
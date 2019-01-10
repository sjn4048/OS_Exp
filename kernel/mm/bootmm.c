#include <arch.h>
#include <driver/vga.h>
#include <zjunix/bootmm.h>
#include <zjunix/utils.h>

// #define BOOTMM_DEBUG

bootmm_t bmm;

unsigned char bootmmap[MACHINE_MMSIZE >> PAGE_SHIFT];

static inline void __set_maps(off_t s_pfn, off_t cnt, unsigned char value)
{
    while (cnt)
    {
        bmm.s_map[s_pfn] = value;
        --cnt;
        ++s_pfn;
    }
}

void set_mminfo(bootmm_info_t *info, off_t start, off_t end, unsigned int type)
{
    info->start = start;
    info->end = end;
    info->type = type;
}

/*
* return value list:
*		0 -> insert_mminfo failed
*		1 -> insert non-related mm_segment
*		2 -> insert forward-connecting segment
*		4 -> insert following-connecting segment
*		6 -> insert forward-connecting segment to after-last position
*		7 -> insert bridge-connecting segment(remove_mminfo is called
for deleting
--
*/
unsigned int insert_mminfo(bootmm_t *mm, off_t start, off_t end, unsigned int type)
{
    for (size_t i = 0; i < mm->info_cnt; i++)
    {
        if (mm->info[i].type != type)
        {
            // ignore dismatch-type infos
            continue;
        }

        if (mm->info[i].end == start - 1)
        {
            // last info of the newly-added info
            if (i + 1 < mm->info_cnt)
            {
                // the newly-added is not the last segment
                if (mm->info[i + 1].type != type)
                {
                    mm->info[i].end = end;
                    return 2;
                }
                else if (mm->info[i + 1].start - 1 == end)
                {
                    mm->info[i].end = mm->info[i + 1].end;
                    remove_mminfo(mm, i + 1);
                    return 7;
                }
            }
            else
            {
                // the newly-added is the last segment
                mm->info[i].end = end;
                return 6;
            }
        }

        if (mm->info[i].start - 1 == end)
        {
#ifdef BOOTMM_DEBUG
            // next info the newly-added info
            kernel_printf("Type of %d: %x, type: %x", i, mm->info[i].type, type);
#endif
            mm->info[i].start = start;
            return 4;
        }
    }

    if (mm->info_cnt >= MAX_INFO)
    {
        // if an info cannot be merged, and it exceeds the MAX_INFO, then fail and return 0
        return 0;
    }

    // the remaining case: individual segment (not-connected to any other same-type segments)
    set_mminfo(mm->info + mm->info_cnt, start, end, type);
    mm->info_cnt++;
    return 1;
}

// remove mm->info[index]
void remove_mminfo(bootmm_t *mm, off_t index)
{
    // check if index is valid
    if (index >= mm->info_cnt)
        return;

    if (index + 1 < mm->info_cnt)
    {
        for (size_t i = index + 1; i < mm->info_cnt; i++)
        {
            mm->info[i - 1] = mm->info[i];
        }
    }
    mm->info_cnt--;
}

unsigned char *find_pages(size_t page_cnt, off_t start_page, off_t end_page, size_t align)
{
    start_page = (start_page + align - 1) & ~(align - 1);
#ifdef BOOTMM_DEBUG
    kernel_printf("Page count: %d, Start page: %d, End page: %d, Align: %d\n", page_cnt, start_page, end_page, align);
#endif // BOOTMM_DEBUG

    // the iteration ends at end_page - page_cnt, as malloc will fail even all the memory left are not used
    for (size_t i = start_page; i <= end_page - page_cnt;)
    {
        size_t cnt = page_cnt;
        size_t tmp = UPPER_ALLIGN(i, align);

        if (bmm.s_map[i] == PAGE_USED)
        {
            i++;
            continue;
        }

        while (cnt)
        {
            if (tmp >= end_page)
            {
                // return NULL if reaches the end
                return 0;
            }

            if (bmm.s_map[tmp] == PAGE_USED)
            {
                // break if page is used.
                break;
            }

            if (bmm.s_map[tmp] == PAGE_FREE)
            {
                tmp++;
                cnt--;
            }
        }

        if (cnt == 0)
        {
            bmm.last_alloc_end = tmp - 1;
            // set used status.
            __set_maps(i, page_cnt, PAGE_USED);
#ifdef BOOTMM_DEBUG
            kernel_printf("Found page at %d\n", i);
#endif // BOOTMM_DEBUG
            return (unsigned char *)(i << PAGE_SHIFT);
        }
        else
        {
            i = tmp;
        }
    }

    // if iteration ended without having found a valid memory, return NULL
    return NULL;
}

// allocate memory in bootmm
// return the absolute address with at least 1 page align
unsigned int bootmm_alloc(size_t size, unsigned int type, size_t align)
{
    // get the page where size is in.
    size_t page_cnt = UPPER_ALLIGN(size, PAGE_TABLE_SIZE) >> PAGE_SHIFT;
#ifdef BOOTMM_DEBUG
    kernel_printf("size: %d, align: %d, need page: %d\n", size, align, page_cnt);
#endif // BOOTMM_DEBUG

    unsigned int res;
    if (res = (unsigned int)find_pages(page_cnt, bmm.last_alloc_end + 1, bmm.max_pfn, align >> PAGE_SHIFT))
    {
        insert_mminfo(&bmm, (unsigned int)res, (unsigned int)res + size - 1, type);
#ifdef BOOTMM_DEBUG
        kernel_printf("bootmm_alloc_pages return: %x\n", res | KERNEL_ENTRY);
#endif // BOOTMM_DEBUG
        return res | KERNEL_ENTRY;
    }

    // if not found by searching forward, then search back.
    if (res = (unsigned int)find_pages(page_cnt, 0, bmm.last_alloc_end, align >> PAGE_SHIFT))
    {
        insert_mminfo(&bmm, (unsigned int)res, (unsigned int)res + size - 1, type);
#ifdef BOOTMM_DEBUG
        kernel_printf("bootmm_alloc_pages(from front) return: %x\n", res | KERNEL_ENTRY);
#endif // BOOTMM_DEBUG
        return res | KERNEL_ENTRY;
    }

// return NULL if not found.
#ifdef BOOTMM_DEBUG
    kernel_printf("bootmm_alloc_pages return NULL\n", res);
#endif // BOOTMM_DEBUG
    return NULL;
}

void init_bootmm()
{
    // use up to 16 MB memory.
    off_t end = 16 * 1024 * 1024;
    kernel_memset(&bmm, 0, sizeof(bmm));
    bmm.phymm = MACHINE_MMSIZE;
    bmm.max_pfn = bmm.phymm >> PAGE_SHIFT;
    bmm.s_map = bootmmap;
    bmm.info_cnt = 0;
    kernel_memset(bmm.s_map, PAGE_FREE, sizeof(bootmmap));
    insert_mminfo(&bmm, 0, end - 1, _MM_KERNEL);
    bmm.last_alloc_end = (end >> PAGE_SHIFT) - 1;

    for (size_t i = 0; i <= bmm.last_alloc_end; i++)
    {
        bmm.s_map[i] = PAGE_USED;
    }
}

void print_bootmap()
{
    char *type_msg[] = {"Kernel code/data", "Mm Bitmap", "Vga Buffer",
                        "Kernel page directory", "Kernel page table", "Dynamic", "Reserved"};
    kernel_printf("Bootmem info:\n");
    for (size_t index = 0; index < bmm.info_cnt; ++index)
    {
        kernel_printf("\t%x-%x : %s\n",
                      bmm.info[index].start, bmm.info[index].end, type_msg[bmm.info[index].type]);
    }
}

void *bootmm_kmalloc(size_t size)
{
    // TODO
}

void bootmm_kfree(void *obj)
{
    // TODO
}

#pragma region temporarily not used

/* get one sequential memory area to be split into two parts
 * (set the former one.end = split_start-1)
 * (set the latter one.start = split_start)
 */
unsigned int split_mminfo(bootmm_t *mm, off_t index, off_t split_start)
{
    off_t start, end;

    start = mm->info[index].start;
    end = mm->info[index].end;
    split_start &= PAGE_ALIGN;

    if ((split_start <= start) || (split_start >= end))
        return 0; // split_start out of range

    if (mm->info_cnt == MAX_INFO)
        return 0; // number of segments are reaching max, cannot alloc anymore
                  // segments
    // using copy and move, to get a mirror segment of mm->info[index]
    for (off_t tmp = mm->info_cnt - 1; tmp >= index; --tmp)
    {
        mm->info[tmp + 1] = mm->info[tmp];
    }
    mm->info[index].end = split_start - 1;
    mm->info[index + 1].start = split_start;
    mm->info_cnt++;
    return 1;
}

// Won't be used actually
void bootmm_free_pages(off_t start, size_t size)
{
    size_t size_inpages;
    off_t index;
    bootmm_info_t target;
    size &= PAGE_ALIGN;
    size_inpages = size >> PAGE_SHIFT;
    if (size_inpages == 0)
    {
#ifdef BOOTMM_DEBUG
        kernel_printf("\tIn bootmm_free_pages: No need to free.\n");
#endif // DEBUG
        return;
    }

    start &= PAGE_ALIGN;
    for (index = 0; index < bmm.info_cnt; index++)
    {
        if (bmm.info[index].start <= start && bmm.info[index].end >= start + size - 1)
        {
            break; // Find the target block
        }
    }
    if (index == bmm.info_cnt)
    {
        kernel_printf("bootmm_free_pages: No allocated space %x-%x\n",
                      start, start + size - 1);
#ifdef BOOTMM_DEBUG
        while (1)
            ;
        return;
#endif
    }

    target = bmm.info[index];
    if (target.start == start)
    {
        if (target.end == start + size - 1)
        {
            remove_mminfo(&bmm, index); // Exactly the same
        }
        else
        {
            // Remove the front part
            set_mminfo(bmm.info + index, start + size, target.end, target.type);
        }
    }
    else if (target.end == start + size - 1)
    {
        // Remove the rear part
        set_mminfo(bmm.info + index, target.start, start - 1, target.type);
    }
    else
    {
        if (split_mminfo(&bmm, index, start) == 0)
        {
            kernel_printf("bootmm_free_pages: split fail\n");
            return;
        }
        set_mminfo(bmm.info + index + 1, start + size, target.end, target.type);
    }
    set_maps(start << PAGE_SHIFT, size_inpages, PAGE_FREE);
}

/*
 * set value of page-bitmap-indicator
 * @param s_pfn	: page frame start node
 * @param cnt	: the number of pages to be set
 * @param value	: the value to be set
 */
void set_maps(off_t s_pfn, size_t cnt, unsigned char value)
{
    while (cnt)
    {
        bmm.s_map[s_pfn] = (unsigned char)value;
        --cnt;
        ++s_pfn;
    }
}

#pragma endregion
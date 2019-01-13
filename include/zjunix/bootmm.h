#ifndef _ZJUNIX_BOOTMM_H
#define _ZJUNIX_BOOTMM_H
#include <zjunix/memory.h>

extern unsigned char __end[];

enum mm_usage
{
    _MM_KERNEL,
    _MM_MMMAP,
    _MM_VGABUFF,
    _MM_PDTABLE,
    _MM_PTABLE,
    _MM_DYNAMIC,
    _MM_RESERVED,
    _MM_COUNT
};

// record every part of mm's information
typedef struct bootmm_info
{
    off_t start;
    off_t end;
    unsigned int type;
} bootmm_info_t;
// typedef bootmm_info_t  * p_mminfo;

// 4K per page
#define PAGE_SHIFT 12
#define PAGE_FREE 0x00
#define PAGE_USED 0xff

#define PAGE_ALIGN (~((1 << PAGE_SHIFT) - 1))

#define MAX_INFO 10

typedef struct bootmm
{
    unsigned int phymm;   // the actual physical memory
    size_t max_pfn; // record the max page number
    unsigned char *s_map; // map begin place
    unsigned int last_alloc_end;
    unsigned int info_cnt; // get number of infos stored in bootmm now
    bootmm_info_t  info[MAX_INFO];
} bootmm_t;
// typedef bootmm_t * p_bootmm;

extern bootmm_t bmm;

extern void set_mminfo(bootmm_info_t *info, unsigned int start, unsigned int end, unsigned int type);

extern unsigned int insert_mminfo(bootmm_t *mm, unsigned int start, unsigned int end, unsigned int type);

extern unsigned int split_mminfo(bootmm_t *mm, unsigned int index, unsigned int split_start);

void* bootmm_kmalloc(size_t size);

void bootmm_kfree(void* obj);

extern void remove_mminfo(bootmm_t *mm, unsigned int index);

extern void init_bootmm();

extern void set_maps(unsigned int s_pfn, unsigned int cnt, unsigned char value);

extern unsigned char *find_pages(unsigned int page_cnt, unsigned int s_pfn, unsigned int e_pfn, unsigned int align_pfn);

extern unsigned int bootmm_alloc(unsigned int size, unsigned int type, unsigned int align);

extern void print_bootmap();

void bootmm_free_pages(unsigned int start, unsigned int size);

#endif

#ifndef _ZJUNIX_BOOTMM_H
#define _ZJUNIX_BOOTMM_H

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
struct bootmm_info
{
    unsigned int start;
    unsigned int end;
    unsigned int type;
};
// typedef struct bootmm_info * p_mminfo;

// 4K per page
#define PAGE_SHIFT 12
#define PAGE_FREE 0x00
#define PAGE_USED 0xff

#define PAGE_ALIGN (~((1 << PAGE_SHIFT) - 1))

#define MAX_INFO 10

struct bootmm
{
    unsigned int phymm;   // the actual physical memory
    unsigned int max_pfn; // record the max page number
    unsigned char *s_map; // map begin place
    unsigned int last_alloc_end;
    unsigned int info_cnt; // get number of infos stored in bootmm now
    struct bootmm_info info[MAX_INFO];
};
// typedef struct bootmm * p_bootmm;

extern struct bootmm bmm;

extern void set_mminfo(struct bootmm_info *info, unsigned int start, unsigned int end, unsigned int type);

extern unsigned int insert_mminfo(struct bootmm *mm, unsigned int start, unsigned int end, unsigned int type);

extern unsigned int split_mminfo(struct bootmm *mm, unsigned int index, unsigned int split_start);

extern void remove_mminfo(struct bootmm *mm, unsigned int index);

extern void init_bootmm();

extern void set_maps(unsigned int s_pfn, unsigned int cnt, unsigned char value);

extern unsigned char *find_pages(unsigned int page_cnt, unsigned int s_pfn, unsigned int e_pfn, unsigned int align_pfn);

extern unsigned int bootmm_alloc(unsigned int size, unsigned int type, unsigned int align);

extern void print_bootmap();

void bootmm_free_pages(unsigned int start, unsigned int size);

#endif

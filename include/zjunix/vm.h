#include <zjunix/buddy.h>

#define VM_DEBUG
#ifdef VM_DEBUG
#include <driver/ps2.h>
#endif

#define USER_CODE_ENTRY 0x0010000
#define USER_DATA_ENTRY 0x0100000
#define USER_DATA_END
#define  USER_BRK_ENTRY     0x10000000
#define  USER_STACK_ENTRY   0x80000000
#define  USER_DEFAULT_ATTR     0x0f

struct mm_struct;

struct vma_struct
{
    struct mm_struct *vm_mm;
    unsigned long vm_start, vm_end;
    struct vma_struct *vm_next;
};

struct mm_struct
{
    struct vma_struct *mmap;
    struct vma_struct *mmap_cache;

    int map_count;
    unsigned int *pgd;

    unsigned int start_code, end_code;
    unsigned int start_data, end_data;
    unsigned int start_brk, brk;
    unsigned int start_stack;
};
#include <zjunix/buddy.h>
#include <zjunix/rbtree.h>
#include <zjunix/memory.h>
#include <zjunix/pc.h>

#define USER_CODE_ENTRY 0x0010000
#define USER_DATA_ENTRY 0x0100000
#define USER_DATA_END
#define USER_BRK_ENTRY 0x10000000
#define USER_STACK_ENTRY 0x80000000
#define USER_DEFAULT_ATTR 0x0f

/* when the count of mm in vma is greater or equal than RB_LIST_THRESHOLD,
 * use RBTREE to search.
 */
#define RB_LIST_THRESHOLD 40

struct mm_struct;

// virtual_memory_area_struct
typedef struct vma_struct
{
    struct mm_struct *vm_mm;
    addr_t vm_start, vm_end;
    struct vma_struct *vm_next;

    // node of Red-Black Tree
    rb_node_t vm_rb;
} vma_t;

typedef struct mm_struct
{
    struct vma_struct *mmap;
    struct vma_struct *mmap_cache;

    // root of Red-Black Tree
    rb_root_t mm_rb;
    size_t map_count;
    pgd_t *pgd;

    addr_t start_code, end_code;
    addr_t start_data, end_data;
    addr_t start_brk, brk;
    addr_t start_stack;

    // owner of struct mm_struct
    task_t *owner;
} mm_t;

static inline void __vma_link_list(mm_t *mm, vma_t *vma, vma_t *prev, rb_node_t *rb_parent);

mm_t *mm_create();

void pgd_delete(pgd_t *pgd);

void exit_mmap(mm_t *mm);

void mm_delete(struct mm_struct *mm);

vma_t *find_vma_contains(mm_t *mm, addr_t addr);

static vma_t *find_vma_intersection(mm_t *mm, addr_t start_addr, addr_t end_addr);

vma_t *find_vma_prev(mm_t *mm, addr_t addr, vma_t **pprev);

addr_t get_unmapped_area(addr_t addr, unsigned long length);

vma_t *find_vma_prepare(mm_t *mm, addr_t addr,
                        vma_t **pprev,
                        rb_node_t ***rb_link, rb_node_t **rb_parent);

void insert_vma_struct(mm_t *mm, vma_t *vma);

addr_t do_map(addr_t addr, size_t length);

int do_unmap(addr_t addr, size_t length);

boolean_t is_in_vma(addr_t addr);
#include <arch.h>
#include <zjunix/vm.h>
#include <zjunix/utils.h>
#include <driver/vga.h>

#pragma region private_func

static inline void __vma_link_list(mm_t *mm, vma_t *vma, vma_t *prev, rb_node_t *rb_parent)
{
    if (prev)
    {
        vma->vm_next = prev->vm_next;
        prev->vm_next = vma;
    }
    else
    {
        mm->mmap = vma;
        if (rb_parent)
            vma->vm_next = rb_entry(rb_parent, vma_t, vm_rb);
        else
            vma->vm_next = NULL;
    }
}

#pragma endregion

mm_t *mm_create()
{
    mm_t *mm = (mm_t *)kmalloc(sizeof(mm_t));

    if (mm == NULL)
    {
#ifdef VMA_DEBUG
        kernel_printf("\tmm_create failed.\n");
#endif
        return NULL;
    }

    kernel_memset(mm, 0, sizeof(mm_t));
    mm->pgd = (pgd_t*)kmalloc(PAGE_SIZE);
    if (mm->pgd == 0)
    {
#ifdef VMA_DEBUG
        kernel_printf("\tmm_create failed as fail to malloc\n");
#endif
        kfree(mm);
        return 0;
    }
    kernel_memset(mm->pgd, 0, PAGE_SIZE);
#ifdef VMA_DEBUG
    kernel_printf("\tmm_create returns mm: %x\n", mm);
#endif
    return mm;
}

// 递归的删除二级页表中的所有内容（先删第二层，然后删第一层，最后删自己）
void pgd_delete(pgd_t *pgd)
{
    for (int i = 0; i < PGD_SPAN; i++)
    {
        unsigned int pde = pgd[i] & PAGE_MASK;

        if (pde == NULL) //不存在二级页表
            continue;
#ifdef VMA_DEBUG
        kernel_printf("Delete pde: %x\n", pde);
#endif //VMA_DEBUG
        unsigned int *pde_ptr = (unsigned int *)pde;
        for (int j = 0; j < PGD_SPAN; j++)
        {
            unsigned int pte = pde_ptr[j] & PAGE_MASK;
            if (pte != NULL)
            {
#ifdef VMA_DEBUG
                kernel_printf("Delete pte: %x\n", pte);
#endif //VMA_DEBUG
                kfree((void*)pte);
            }
        }
        kfree((void*)pde_ptr);
    }
    kfree(pgd);
#ifdef VMA_DEBUG
    kernel_printf("pgd_delete done.\n");
#endif //VMA_DEBUG
    return;
}

// Free all the vmas
void exit_mmap(mm_t *mm)
{
    vma_t *vmap = mm->mmap;
    mm->mmap = mm->mmap_cache = 0;
    while (vmap)
    {
        vma_t *next = vmap->vm_next;
        kfree(vmap);
        vmap = next;
        mm->map_count--;
    }
    if (mm->map_count)
    {
        kernel_printf("exit_mmap() exited unexpectedly. %d vma left", mm->map_count);
        // Unexpected BUG
        while (1)
            ;
    }
}

// 先删pgd, 再删vma，最后kfree掉自己
void mm_delete(struct mm_struct *mm)
{
#ifdef VMA_DEBUG
    kernel_printf("mm_delete: mm: %x, pgd %x\n", mm, mm->pgd);
#endif //VMA_DEBUG
    pgd_delete(mm->pgd);
    exit_mmap(mm);
#ifdef VMA_DEBUG
    kernel_printf("exit_mmap done\n");
#endif //VMA_DEBUG
    kfree(mm);
}

/*************VMA*************/
// Find the first vma with ending address greater than addr
// Finish RBTree
vma_t *find_vma_contains(mm_t *mm, addr_t addr)
{
    if (mm == NULL)
    {
        kernel_printf("\tIn find_vma_contains(): mm is null.");
    }

    vma_t *vma = NULL;
    if (mm->map_count < RB_LIST_THRESHOLD)
    {
        // use list to search
        // if in cache directly
        vma = mm->mmap_cache;
        if (vma != NULL && vma->vm_end > addr && vma->vm_start <= addr)
        {
            return vma;
        }

        vma = mm->mmap;
        while (vma != NULL)
        {
            if (vma->vm_end > addr)
            {
                mm->mmap_cache = vma;
                break;
            }
            vma = vma->vm_next;
        }
    }
    else
    {
        vma = mm->mmap_cache;
        if (!(vma && vma->vm_end > addr && vma->vm_start <= addr))
        {
            rb_node_t *rb_node;

            rb_node = mm->mm_rb.rb_node;
            vma = NULL;

            while (rb_node)
            {
                vma_t *vma_tmp;

                vma_tmp = rb_entry(rb_node, vma_t, vm_rb);

                if (vma_tmp->vm_end > addr)
                {
                    vma = vma_tmp;
                    if (vma_tmp->vm_start <= addr)
                        break;
                    rb_node = rb_node->rb_left;
                }
                else
                    rb_node = rb_node->rb_right;
            }
            if (vma)
                mm->mmap_cache = vma;
        }
    }

    // return vma whatsoever.
    return vma;
}

// Find the first vma overlapped with start_addr~end_addr
// which as, ret.start >= end_addr, ret.end >= start_addr
// No need to RBTree
static vma_t *find_vma_intersection(mm_t *mm, addr_t start_addr, addr_t end_addr)
{
    vma_t *vma = find_vma_contains(mm, start_addr);
    if (vma && end_addr <= vma->vm_start)
    {
        vma = NULL;
    }
    return vma;
}

// Return the first vma with ending address greater than addr, recording the previous vma
// Finish RBTree
vma_t *find_vma_prev(mm_t *mm, addr_t addr, vma_t **pprev)
{
    vma_t *vma = NULL;
    *pprev = NULL;

    if (mm == NULL)
    {
        *pprev = NULL;
        kernel_printf("\tIn find_vma_prev(): mm is null.");
        return NULL;
    }

    // if map_count < THRESHOLD, use list to handle.
    if (mm->map_count < RB_LIST_THRESHOLD)
    {
        vma = mm->mmap;
        while (vma)
        {
            if (vma->vm_end > addr)
            {
                mm->mmap_cache = vma;
                break;
            }
            *pprev = vma;
            vma = vma->vm_next;
        }
    }
    else
    {
        /* Go through the RB tree quickly. */
        vma_t *vma;
        rb_node_t *rb_node, *rb_last_right, *rb_prev;

        rb_node = mm->mm_rb.rb_node;
        rb_last_right = rb_prev = NULL;
        vma = NULL;

        while (rb_node)
        {
            vma_t *vma_tmp;

            vma_tmp = rb_entry(rb_node, vma_t, vm_rb);

            if (vma_tmp->vm_end > addr)
            {
                vma = vma_tmp;
                rb_prev = rb_last_right;
                if (vma_tmp->vm_start <= addr)
                    break;
                rb_node = rb_node->rb_left;
            }
            else
            {
                rb_last_right = rb_node;
                rb_node = rb_node->rb_right;
            }
        }
        if (vma)
        {
            if (vma->vm_rb.rb_left)
            {
                rb_prev = vma->vm_rb.rb_left;
                while (rb_prev->rb_right)
                {
                    rb_prev = rb_prev->rb_right;
                }
            }
            *pprev = NULL;
            if (rb_prev)
            {
                *pprev = rb_entry(rb_prev, vma_t, vm_rb);
            }
            if ((rb_prev ? (*pprev)->vm_next : mm->mmap) != vma)
            {
                kernel_printf("\tIn insert_vma_struct: ERROR: vma's end < start.\n");
                while (1)
                    ;
            }
            return vma;
        }
    }

    return vma;
}

// Get unmapped area starting after addr        目前省略了file, pgoff
// Refactored from Linux.
addr_t get_unmapped_area(addr_t addr, unsigned long length)
{
    vma_t *vma;
    mm_t *mm = current_task->mm; //全局变量，当前线程对应的task_struct

    addr = UPPER_ALLIGN(addr, PAGE_SIZE); // Allign to page size
    if (addr + length > KERNEL_ENTRY)
    {
        kernel_printf("\tIn get_unmapped_area(): addr + length > KERNEL_ENTRY");
        return -1;
    }

    if (addr != NULL)
    {
        for (vma = find_vma_contains(mm, addr);; vma = vma->vm_next)
        {
            if (addr + length > KERNEL_ENTRY)
                return -1;
            if (vma == NULL || addr + length <= vma->vm_start)
            {
                return addr;
            }
            addr = vma->vm_end;
        }
    }

    return 0;
}

// Finish RBTree
vma_t *find_vma_prepare(mm_t *mm, addr_t addr,
                        vma_t **pprev,
                        rb_node_t ***rb_link, rb_node_t **rb_parent)
{
    vma_t *vma;
    // for the situation of small list:
    if (mm->map_count <= RB_LIST_THRESHOLD)
    {
        vma = mm->mmap;
        vma_t *prev = NULL;
        while (vma != NULL)
        {
            if (addr < vma->vm_start)
            {
                break;
            }

            prev = vma;
            vma = vma->vm_next;
        }
        return prev;
    }
    else // else, do the RBTree part
    {
        rb_node_t **__rb_link, *__rb_parent, *rb_prev;

        __rb_link = &mm->mm_rb.rb_node;
        rb_prev = __rb_parent = NULL;
        vma = NULL;

        while (*__rb_link)
        {
            vma_t *vma_tmp;

            __rb_parent = *__rb_link;
            vma_tmp = rb_entry(__rb_parent, vma_t, vm_rb);

            if (vma_tmp->vm_end > addr)
            {
                vma = vma_tmp;
                if (vma_tmp->vm_start <= addr)
                    return vma;
                __rb_link = &__rb_parent->rb_left;
            }
            else
            {
                rb_prev = __rb_parent;
                __rb_link = &__rb_parent->rb_right;
            }
        }

        *pprev = NULL;
        if (rb_prev)
            *pprev = rb_entry(rb_prev, vma_t, vm_rb);
        *rb_link = __rb_link;
        *rb_parent = __rb_parent;
        return vma;
    }
}

// Insert vma to the linked list
// finished adding RBTree
void insert_vma_struct(mm_t *mm, vma_t *vma)
{
    // first, insert area into mm's list.
    vma_t *__vma, *prev;
    rb_node_t **rb_link, *rb_parent;
    __vma = find_vma_prepare(mm, vma->vm_start, &prev, &rb_link, &rb_parent);
    if (__vma && __vma->vm_start < vma->vm_end)
#ifdef VMA_DEBUG
    kernel_printf("Insert: %x  %x", vma, vma->vm_start);
#endif //VMA_DEBUG
    if (vma == NULL)
    {
#ifdef VMA_DEBUG
        kernel_printf("mmap init\n");
#endif //VMA_DEBUG
        mm->mmap = vma;
        vma->vm_next = 0;
    }
    else
    {
        if (vma->vm_start < vma->vm_end)
        {
            kernel_printf("\tIn insert_vma_struct: ERROR: vma's end < start.\n");
            while (1)
                ;
        }
        vma->vm_next = vma->vm_next;
        vma->vm_next = vma;
    }

    // then, insert area into mm's RBTree;
    if (__vma && __vma->vm_start < vma->vm_end)
    {
        kernel_printf("\tIn insert_vma_struct: ERROR: vma's end < start.\n");
        while (1)
            ;
    }
    // TODO: may be duplicate.
    __vma_link_list(mm, vma, prev, rb_parent);

    // finally, increment count
    mm->map_count++;
}

// do mapping a region
addr_t do_map(addr_t addr, size_t length)
{
    mm_t *mm = current_task->mm;
    vma_t *vma;
    if (length == 0)
    {
        return addr;
    }

    addr = get_unmapped_area(addr, length);
    vma = kmalloc(sizeof(vma_t));
    if (vma == NULL)
    {
        kernel_printf("\tIn do_map(): ERROR because kmalloc failed.\n");
        while (1)
            ;
    }

    vma->vm_mm = mm;
    vma->vm_start = addr;
    vma->vm_end = UPPER_ALLIGN(addr + length, PAGE_SIZE);
#ifdef VMA_DEBUG
    kernel_printf("Do map: %x  %x\n", vma->vm_start, vma->vm_end);
#endif //VMA_DEBUG
    insert_vma_struct(mm, vma);
    return addr;
}

int do_unmap(addr_t addr, size_t length)
{
    mm_t *mm = current_task->mm;
    vma_t *vma, *prev;
    if (addr > KERNEL_ENTRY || length + addr > KERNEL_ENTRY)
    {
#ifdef VMA_DEBUG
        kernel_printf("\tIn do_unmap(): bad_addr %x, length %d", addr, length);
#endif             //VMA_DEBUG
        return -1; // Bad addr
    }

    vma = find_vma_prev(mm, addr, &prev);
    if (vma == NULL || vma->vm_start >= addr + length)
    {
#ifdef VMA_DEBUG
        kernel_printf("\tIn do_unmap(): %x has not been mapped.", addr);
#endif            //VMA_DEBUG
        return 0; // It has not been mapped
    }
#ifdef VMA_DEBUG
    kernel_printf("do_unmap. %x %x\n", addr, vma->vm_start);
#endif //VMA_DEBUG
    // VMA Length match
    if (vma->vm_end >= addr + length)
    {
#ifdef VMA_DEBUG
        kernel_printf("Length match\n");
#endif //VMA_DEBUG
        if (prev == NULL)
        {
            mm->mmap = vma->vm_next;
        }
        else
        {
            prev->vm_next = vma->vm_next;
        }
        kfree(vma);
        mm->map_count--;
#ifdef VMA_DEBUG
        kernel_printf("Unmap done. %d vma(s) left\n", mm->map_count);
#endif // VMA_DEBUG
        return 0;
    }

    // Length mismatch
#ifdef VMA_DEBUG
    kernel_printf("\tIn do_unmap(): Length match");
#endif //VMA_DEBUG
    return 1;
}

boolean_t is_in_vma(addr_t addr)
{
    mm_t *mm = current_task->mm;
    vma_t *vma = find_vma_contains(mm, addr);
    if (vma == NULL || vma->vm_start > addr)
    {
        return False;
    }
    else
    {
        return True;
    }
}
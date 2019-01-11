#include "exc.h"

#include <arch.h>
#include <driver/vga.h>
#include <driver/ps2.h>
#include <zjunix/pc.h>
#include <zjunix/memory.h>
#include <zjunix/vm.h>

#pragma GCC push_options
#pragma GCC optimize("O0")

exc_fn exceptions[32];
extern int in_stress_testing;
void tlb_refill(unsigned int bad_addr)
{
    pgd_t *pgd;
    off_t pde_index, pte_index, pte_index_near;
    unsigned int pde, pte, pte_near;
    unsigned int pte_phy, pte_near_phy;
    unsigned int *pde_ptr, pte_ptr;
    unsigned int entry_lo0, entry_lo1, entry_hi;

#ifdef TLB_DEBUG
    unsigned int entry_hi_res;

    asm volatile(
        "mfc0  $t0, $10\n\t"
        "move  %0, $t0\n\t"
        : "=r"(entry_hi_res));

    kernel_printf("\tIn tlb_refill: bad_addr=%x, entry_hi=%x, current task: %x(%d)\n", bad_addr, entry_hi_res, current_task, current_task->PID);
#endif

    if (current_task->mm == NULL)
    {
        kernel_printf("\t[ERROR]: mm is null!!!  %d\n", current_task->PID);
        goto error_null_or_not_exist;
    }

    // retrieve the page table entry of current task
    pgd = current_task->mm->pgd;
    if (pgd == NULL)
    {
        kernel_printf("\t[ERROR]: mm is null!!!  %d\n");
        goto error_null_or_not_exist;
    }
    // retrive the page_base address of bad_addr
    bad_addr &= PAGE_MASK;

    // judge if bad_addr is in vma
    if (is_in_vma(bad_addr) == False)
    {
        kernel_printf("\t[ERROR]: bad_addr is not in vma of current process!  %d\n");
        goto error_null_or_not_exist;
    }

    pde_index = bad_addr >> PGD_SHIFT;
    pde = pgd[pde_index] & PAGE_MASK;

    if (pde == NULL)
    {
#ifdef TLB_DEBUG
        kernel_printf("\tSecond page table not exist.\n");
#endif
        // 二级页表不存在
        pde = (unsigned int)kmalloc(PAGE_SIZE);

        if (pde == NULL)
        {
            kernel_printf("\tTlb_refill: alloc second page table failed.\n");
            goto error_null_or_not_exist;
        }

        kernel_memset((void *)pde, 0, PAGE_SIZE);
        pgd[pde_index] = (pde & PAGE_MASK) | 0x0f; // attr
    }
#ifdef VMA_DEBUG
    kernel_printf("\tTlb refill: %x\n", pde);
#endif
    pde_ptr = (unsigned int *)pde;
    pte_index = (bad_addr >> PAGE_SHIFT) & INDEX_MASK;
    pte = pde_ptr[pte_index] & PAGE_MASK;
    if (pte == NULL)
    {
#ifdef TLB_DEBUG
        kernel_printf("\tIn second pte: page not exist\n");
#endif
        pte = (unsigned int)kmalloc(PAGE_SIZE); //要考虑物理地址？？？

        if (pte == NULL)
        {
            kernel_printf("\tTlb_refill: alloc page failed!\n");
            goto error_null_or_not_exist;
        }

        kernel_memset((void *)pte, 0, PAGE_SIZE);
        pde_ptr[pte_index] = (pte & PAGE_MASK) | 0x0f;
    }

    // 旁边的那一页
    pte_index_near = pte_index ^ 0x1;
    pte_near = pde_ptr[pte_index_near] & PAGE_MASK;

#ifdef TLB_DEBUG
    kernel_printf("pte: %x pte_index: %x  pte_near_index: %x\n", pte, pte_index, pte_index_near);
#endif

    if (pte_near == NULL)
    { //附近项 为空
#ifdef TLB_DEBUG
        kernel_printf("\tIn Tlb_refill: page near not exist\n");
#endif

        pte_near = (unsigned int)kmalloc(PAGE_SIZE);

        if (pte_near == NULL)
        {
            kernel_printf("\tTlb_refill: alloc pte_near failed!\n");
            goto error_null_or_not_exist;
        }

        kernel_memset((void *)pte_near, 0, PAGE_SIZE);
        pde_ptr[pte_index_near] = (pte_near & PAGE_MASK) | 0x0f;
    }

    //换成物理地址
    pte_phy = pte - KERNEL_ENTRY;
    pte_near_phy = pte_near - KERNEL_ENTRY;
#ifdef TLB_DEBUG
    kernel_printf("Allocated pte: %x  %x\n", pte_phy, pte_near_phy);
#endif

    if (pte_index & 0x01 == 0)
    {
        // 如果pte是偶数
        entry_lo0 = (pte_phy >> PAGE_SHIFT) << 6;
        entry_lo1 = (pte_near_phy >> PAGE_SHIFT) << 6;
    }
    else
    {
        entry_lo0 = (pte_near_phy >> PAGE_SHIFT) << 6;
        entry_lo1 = (pte_near >> PAGE_SHIFT) << 6;
    }
    entry_lo0 |= (3 << 3); //cached ??
    entry_lo1 |= (3 << 3); //cached ??
    entry_lo0 |= 0x06;     //D = 1, V = 1, G = 0
    entry_lo1 |= 0x06;

    entry_hi = (bad_addr & PAGE_MASK) & (~(1 << PAGE_SHIFT));
    entry_hi |= current_task->ASID;

#ifdef TLB_DEBUG
    kernel_printf("pid: %d\n", current_task->PID);
    kernel_printf("tlb_refill: entry_hi: %x  entry_lo0: %x  entry_lo1: %x\n", entry_hi, entry_lo0, entry_lo1);
#endif

    asm volatile(
        "move $t0, %0\n\t"
        "move $t1, %1\n\t"
        "move $t2, %2\n\t"
        "mtc0 $t0, $10\n\t"
        "mtc0 $zero, $5\n\t"
        "mtc0 $t1, $2\n\t"
        "mtc0 $t2, $3\n\t"
        "nop\n\t"
        "nop\n\t"
        "tlbwr\n\t"
        "nop\n\t"
        "nop\n\t"
        :
        : "r"(entry_hi),
          "r"(entry_lo0),
          "r"(entry_lo1));

    kernel_printf("after refill\n");
    unsigned int *pgd_ = current_task->mm->pgd;
    unsigned int pde_, pte_;
    unsigned int *pde_ptr_;

    for (size_t i = 0; i < PGD_SPAN; i++)
    {
        pde_ = pgd_[i] & PAGE_MASK;

        if (pde_ == NULL)
        {
            //不存在二级页表
            continue;
        }

        kernel_printf("pde: %x\n", pde_);
        pde_ptr_ = (unsigned int *)pde_;
        for (size_t j = 0; j < PGD_SPAN; j++)
        {
            pte_ = pde_ptr_[j] & PAGE_MASK;
            if (pte_ != NULL)
            {
                kernel_printf("\tpte: %x\n", pte_);
            }
        }
    }
    // if (count_2 == 4) {
    //     kernel_printf("")
    //     while(1)
    //         ;
    // }

    return;

error_null_or_not_exist:
    while (1)
        ;
}

void do_exceptions(unsigned int status, unsigned int cause, context *pt_context, unsigned int bad_addr)
{
    int index = cause >> 2;
    index &= 0x1f;

    //     if (index == 2 || index == 3)
    //     {
    //         // if encountered TLB miss
    // #ifdef TLB_DEBUG
    //         kernel_printf("Start TLB_Refill. Index: %d, bad_addr: %x, status: %d, cause:%d, epc=%x\n", index, bad_addr, status, cause);
    // #endif
    //         tlb_refill(bad_addr);
    // #ifdef TLB_DEBUG
    //         kernel_printf("TLB_Refill done.\n");
    // #endif
    //     }

    if (exceptions[index])
    {
        exceptions[index](status, cause, pt_context);
    }
    else
    {
        kernel_printf_color(VGA_RED, "[Exception Interrupt]\n");
        task_struct *pcb;
        unsigned int badVaddr;
        asm volatile("mfc0 %0, $8\n\t"
                     : "=r"(badVaddr));
        pcb = current_task;
        // if (current_task)
        // {
        if (in_stress_testing == 0)
        {
            kernel_printf("\nProcess %s exited due to exception cause=%x;\n", pcb->name, cause);
            kernel_printf("status=%x, EPC=%x, BadVaddr=%x\n", status, pcb->context.epc, badVaddr);
        }
        pc_kill_syscall(status, cause, pt_context);
        // }
    }
}

void register_exception_handler(int index, exc_fn fn)
{
    index &= 31;
    exceptions[index] = fn;
}

void init_exception()
{
    // status 0000 0000 0000 0000 0000 0000 0000 0000
    // cause 0000 0000 1000 0000 0000 0000 0000 0000
    asm volatile(
        "mtc0 $zero, $12\n\t"
        "li $t0, 0x800000\n\t"
        "mtc0 $t0, $13\n\t");
}

#pragma GCC pop_options
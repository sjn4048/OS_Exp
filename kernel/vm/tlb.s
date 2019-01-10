# Copied from chenyuan.

.globl  set_tlb_asid

.set noreorder
.set noat
.align 2

set_tlb_asid:
    mtc0    $zero, $5   #PageMask
    mfc0    $t0, $10    #entry_hi
    li      $t1, 0xffffe000
    and     $t0, $t0, $t1
    or      $t0, $t0, $a0
    mtc0    $t0, $10
    nop
    nop
    jr      $ra

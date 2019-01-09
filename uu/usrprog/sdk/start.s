.globl entry
.set noreorder
.set noat
.align 2

.org 0x0
entry:
	la $t0, sdk_init
	jal $t0
	nop
	nop
	jr $ra
	nop
	nop



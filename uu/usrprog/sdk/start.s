.globl entry
.set noreorder
.set noat
.align 2

.org 0x0
entry:
	addi $gp, $gp, 10000
	la $t0, sdk_init
	jal $t0
	nop
	nop
	addi $gp, $gp, -10000




# .globl entry
# .set noreorder
# .set noat
# .align 2

# .org 0x0
# entry:
# 	# program's address transformation
# 	la $t0, sdk_init
# 	add $t0, $t0, $a2
# 	# jump to main entry and not set $ra
# 	# so when main func returns , it directly returns back to OS 
# 	j $t0
# 	nop

.globl entry
entry:
mtc0 $a2, $7
nop
nop
# jal main
mtc0 $zero, $7
nop
nop
jr $ra
nop
nop

.org 0x100
main:
li $v0, 5
jr $ra
nop
nop


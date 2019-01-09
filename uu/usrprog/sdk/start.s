.globl entry
.set noreorder
.set noat
.align 2

.org 0x0
entry:
	# program's address transformation
	la $t0, sdk_init
	
	# ----------------------
	# add $t0, $t0, $a2
	# ----------------------

	# jump to main entry and not set $ra
	# so when main func returns , it directly returns back to OS 
	j $t0
	nop

	.text
	.globl setup_mmu

setup_mmu:
	ld a0, lvl0_ppn
	slli a0, a0, 10
	addi a0, a0, 1
	li a1, 0x80100000
	li t0, 0
	li t1, 512
lvl0:
	sd a0, 0(a1)
	addi a1, a1, 8
	addi t0, t0, 1
	blt t0, t1, lvl0

	#setup io mem ppn2
	ld a0, lvl0_io_ppn
	slli a0, a0, 10
	addi a0, a0, 1
	li a1, 0x80100000
	sd a0, 0(a1)

	#setup user ppn2
	ld a0, lvl0_user_ppn
	slli a0, a0, 10
	addi a0, a0, 0x1
	#addi a0, a0, 0x11
	li a1, 0x80100000
	sd a0, 8(a1)


	ld a0, lvl1_ppn
	slli a0, a0, 10
	addi a0, a0, 1
	li a1, 0x80101000
	li t0, 0
	li t1, 512
lvl1:
	sd a0, 0(a1)
	addi a1, a1, 8
	addi t0, t0, 1
	blt t0, t1, lvl1


	#setup io mem ppn1
	ld a0, lvl1_io_ppn
	slli a0, a0, 10
	addi a0, a0, 1
	li a1, 0x80103000
	li t0, 0
	li t1, 512
lvl1_io:
	sd a0, 0(a1)
	addi a1, a1, 8
	addi t0, t0, 1
	blt t0, t1, lvl1_io

	#setup user mem ppn1
	ld a0, lvl1_user_ppn
	slli a0, a0, 10
	addi a0, a0, 0x1
	#addi a0, a0, 0x11
	li a1, 0x80105000
	li t0, 0
	li t1, 512
lvl1_user:
	sd a0, 0(a1)
	addi a1, a1, 8
	addi t0, t0, 1
	blt t0, t1, lvl1_user



	ld a0, lvl2_ppn
	slli a0, a0, 10
	addi a0, a0, 0xf
	li a1, 0x80102000
	li t0, 0
	li t1, 512
lvl2:
	sd a0, 0(a1)
	addi a1, a1, 8
	addi t0, t0, 1
	addi a0, a0, 1024
	blt t0, t1, lvl2

	#setup io mem ppn0
	ld a0, lvl2_io_ppn
	slli a0, a0, 10
	addi a0, a0, 0xf
	li a1, 0x80104000
	li t0, 0
	li t1, 512
lvl2_io:
	sd a0, 0(a1)
	addi a1, a1, 8
	addi t0, t0, 0x1
	addi a0, a0, 1024
	blt t0, t1, lvl2_io

	#setup user mem ppn0
	ld a0, lvl2_user_ppn
	slli a0, a0, 10
	#addi a0, a0, 0xf
	addi a0, a0, 0x1f
	li a1, 0x80106000
	li t0, 0
	li t1, 512
lvl2_user:
	sd a0, 0(a1)
	addi a1, a1, 8
	addi t0, t0, 0x1
	addi a0, a0, 1024
	blt t0, t1, lvl2_user


mmu_jump:
	li a0, 0x80100
	#slli a0, a0, 2
	li t0, 0x8000000000000000
	add a0, a0, t0 
	csrw satp, a0
	li a0, 0x80100
	sfence.vma
	ret

	.globl switch_satp 
switch_satp:
	csrw satp, a0
	ret

lvl0_ppn:
	.dword 0x80101
lvl1_ppn:
	.dword 0x80102
lvl2_ppn:
	.dword 0x80000

lvl0_io_ppn:
	.dword 0x80103
lvl1_io_ppn:
	.dword 0x80104
lvl2_io_ppn:
	.dword 0x10000


lvl0_user_ppn:
	.dword 0x80105
lvl1_user_ppn:
	.dword 0x80106
lvl2_user_ppn:
	.dword 0x80000

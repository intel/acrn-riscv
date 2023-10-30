	.section .app.text
	.global app
app:
#	la a0, user_msg
#	call printk
#	li a0, 0
#	ecall
#
	li t0, 0xff
	move a0, t0
	move a0, t0
	move a0, t0
	nop
	nop
	ecall

user_msg:
	.string "i'm app\n"

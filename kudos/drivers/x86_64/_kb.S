/*
 * x86 Keyboard handling
 */
.code64

/* IRQ Keyboard (PS2) */
.global keyboard_irq_handler
.global _keyboard_wait

.extern keyboard_key_int
.extern pic_eoi

_keyboard_wait:
	pause
	/*hlt*/
	ret

keyboard_irq_handler:
	 /* Disable interrupts */
	cli

	/* Save registers */
	mov %r15, -0x8(%rsp)
	mov %r14, -0x10(%rsp)
	mov %r13, -0x18(%rsp)
	mov %r12, -0x20(%rsp)
	mov %r11, -0x28(%rsp)
	mov %r10, -0x30(%rsp)
	mov %r9,  -0x38(%rsp)
	mov %r8,  -0x40(%rsp)
	mov %rdi, -0x48(%rsp)
	mov %rsi, -0x50(%rsp)
	mov %rbp, -0x58(%rsp)
	mov %rsp, -0x60(%rsp)
	mov %rbx, -0x68(%rsp)
	mov %rdx, -0x70(%rsp)
	mov %rcx, -0x78(%rsp)
	mov %rax, -0x80(%rsp)
	sub $0x80, %rsp

	/* Do stuff! */
	call keyboard_key_int
	
	/* Acknowledge irq */
	mov $1, %rdi
	call pic_eoi

	/* Restore */
	add $0x80, %rsp
	mov -0x8(%rsp), %r15
	mov -0x10(%rsp), %r14
	mov -0x18(%rsp), %r13
	mov -0x20(%rsp), %r12
	mov -0x28(%rsp), %r11
	mov -0x30(%rsp), %r10
	mov -0x38(%rsp), %r9
	mov -0x40(%rsp), %r8
	mov -0x48(%rsp), %rdi
	mov -0x50(%rsp), %rsi
	mov -0x58(%rsp), %rbp
	mov -0x68(%rsp), %rbx
	mov -0x70(%rsp), %rdx
	mov -0x78(%rsp), %rcx
	mov -0x80(%rsp), %rax

	/* Reenable interrupts */
	sti

	/* Return */
	iretq

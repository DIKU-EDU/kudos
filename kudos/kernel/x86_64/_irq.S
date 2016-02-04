/*
 * Irq Assembly Rutines
 */
.code64

/* Function Definitions */
.global isr_default_handler
.global _idle_thread_wait_loop
.global _timer_set_ticks
.global yield_irq_handler
.global __enable_irq
.global __disable_irq
.global __getflags
.global isr_handler0
.global isr_handler1
.global isr_handler2
.global isr_handler3
.global isr_handler4
.global isr_handler5
.global isr_handler6
.global isr_handler7
.global isr_handler8
.global isr_handler9
.global isr_handler10
.global isr_handler11
.global isr_handler12
.global isr_handler13
.global isr_handler14
.global isr_handler15
.global isr_handler16
.global isr_handler17
.global isr_handler18
.global isr_handler19

/* Software Interrupts */
.global _interrupt_yield

isr_default_handler:
	iretq

_interrupt_yield:
	int $0x81
	ret

_idle_thread_wait_loop:
	hlt
	jmp _idle_thread_wait_loop

_timer_set_ticks:
	ret

__enable_irq:
	sti
	ret

__disable_irq:
	cli
	ret

__getflags:
	pushfq
	pop %rax
	ret

/* IRQ Macros */
.macro PUSHAQ
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
.endm

.macro POPAQ
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
.endm

/* Yield */
.extern pic_eoi
.extern task_switch

yield_irq_handler:
	 /* Disable interrupts */
	cli

	/* Save registers */
	PUSHAQ

	/* Switch task */
	mov %rsp, %rdi
	call task_switch

	/* It returns a new stack for us in rax */
	mov %rax, %rsp

	/* Acknowledge irq */
	mov $0, %rdi
	call pic_eoi

	/* Restore */
	POPAQ

	/* Reenable interrupts */
	sti

	/* Return */
	iretq

/* Common Entry */
.extern interrupt_handle
.global IsrCommon

IsrCommon:
	/* Disable interrupts */
	cli

	/* Save registers */
	PUSHAQ
	
	/* Mov info */
	mov %rsp, %rdi

	/* Call common entry */
	call interrupt_handle

	/* Restore */
	POPAQ

	/* Reenable interrupts */
	sti

	/* Return */
	add $16, %rsp
	iretq

/* IRQS */
isr_handler0:
	pushq 0
	pushq 0
	jmp IsrCommon

isr_handler1:
	pushq 0
	pushq 1
	jmp IsrCommon

isr_handler2:
	pushq 0
	pushq 2
	jmp IsrCommon

isr_handler3:
	pushq 0
	pushq 3
	jmp IsrCommon

isr_handler4:
	pushq 0
	pushq 4
	jmp IsrCommon

isr_handler5:
	pushq 0
	pushq 5
	jmp IsrCommon

isr_handler6:
	pushq 0
	pushq 6
	jmp IsrCommon

isr_handler7:
	pushq 0
	pushq 7
	jmp IsrCommon

isr_handler8:
	pushq 8
	jmp IsrCommon

isr_handler9:
	pushq 0
	pushq 9
	jmp IsrCommon

isr_handler10:
	pushq 10
	jmp IsrCommon

isr_handler11:
	pushq 11
	jmp IsrCommon

isr_handler12:
	pushq 12
	jmp IsrCommon

isr_handler13:
	pushq 13
	jmp IsrCommon

isr_handler14:
	hlt
	pushq 14
	jmp IsrCommon

isr_handler15:
	pushq 0
	pushq 15
	jmp IsrCommon

isr_handler16:
	pushq 0
	pushq 16
	jmp IsrCommon

isr_handler17:
	pushq 17
	jmp IsrCommon

isr_handler18:
	pushq 0
	pushq 18
	jmp IsrCommon

isr_handler19:
	pushq 0
	pushq 19
	jmp IsrCommon

/*
 * Tools Assembly
 */
.code64

.global _inb
.global _outb
.global __stack_chk_fail

_inb:
	/* RDI has port */
	pushq %rdx

	/* Clear */
	xor %rax, %rax
	xor %rdx, %rdx

	/* Mov values */
	movw %di, %dx
	inb %dx, %al

	popq %rdx
	ret

_outb:
	/* RDI has port, RSI has value */
	pushq %rax
	pushq %rdx

	/* Clear */
	xor %rax, %rax
	xor %rdx, %rdx

	/* Mov values */
	movw %si, %ax
	movw %di, %dx
	outb %al, %dx

	popq %rdx
	popq %rax
	ret

__stack_chk_fail:
  ret

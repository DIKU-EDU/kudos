/*
 * Spinlock implementation
 */
.code64

/* Acquire by spinning */
/* */
.global spinlock_acquire

spinlock_acquire:
	/* Setup for locked exchange */
	push %rax
	push %rbx
	mov $0x1, %rax
	mov %rdi, %rbx

	/* Xchg instruction is atomic */
	xchg %eax, (%ebx)
	test %eax, %eax
	je lock_acquired

	/* Acquire Loop */
acquire_loop:
	pause
	cmp $0, (%ebx)
	jne acquire_loop

	/* Done! */
lock_acquired:
	pop %rbx
	pop %rax
	ret

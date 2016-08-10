/*
 * The boot assembler code (linker entry point is _start).
 */

 /* Put us in front (multiboot header)
  * needs to be in first 8kb */
.section ".boot"

/* We get loaded in Pmode by GRUB (32 bits) */
.code32

/* Constants */
.set PDE_ADDRESS,	0x64000  /* PDE*/
.set PDPTE_ADDRESS,	0x62000  /* PDPTE */
.set PML4_ADDRESS,	0x60000  /* PML4 */

.set PAGE_PRESENT,	0x1
.set PAGE_WRITE,	0x2
.set PAGE_2MB,		0x80

.set PAGE_FLAGS,	(PAGE_PRESENT | PAGE_WRITE | PAGE_2MB)

MultibootHeader:
	.long 0x1badb002			/* Multiboot Magic Nr */
	.long 2						/* Flags */
	.long -(0x1badb002 + 2)		/* Checksum */
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0

.section ".text"
.global _start

.align 16
TempGDT32:
	.quad 0x0000000000000000 /* Null Segment */
	.quad 0x00cf9a000000ffff /* Code Segment */
	.quad 0x00cf92000000ffff /* Data Segment */

.align 16
TempGDT64:
	.quad 0x0000000000000000 /* Null Segment */
	.quad 0x00a09a0000000000 /* Code Segment */
	.quad 0x00a0920000000000 /* Data Segment */

.align 16
GDTR32:
	.word 23
	.long TempGDT32

.align 16
GDTR64:
	.word 23
	.quad TempGDT64

_start:
	/* Disable Ints */
	cli

	/* Save Multiboot info (eax, ebx)*/
	pushl $0
	pushl %eax   /* Simulate 64 bit push, we are in 32 bit */
	pushl $0
	pushl %ebx

	/* Check if CPU supports 64 bit (long mode) */
	movl $0x80000000, %eax
	cpuid
	cmpl $0x80000000, %eax   /* test if we have extended features */
	jbe 4f
	movl $0x80000001, %eax
	cpuid					/* edx now contains feature flags */
	bt $29, %edx			/* Bit 29 is IA64 bit flag */
	jnc 4f

	/* Load 32 Bit GDT - Fix this */
	lgdt (GDTR32)
	
	/* Clear CS */
	ljmp $0x8, $1f

	/* If we reach here, congrats, 64 bit is avail */
1:
	/* Setup segments */
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %gs
	movw %ax, %fs
	movw %ax, %ss

	/* Clear registers */
	xor %eax, %eax
	xor %ebx, %ebx
	xor %ecx, %ecx
	xor %edx, %edx

	/* First step - Setup initial paging */

	/* Identity PDE With 2 MB Pages */
	movl $PDE_ADDRESS, %eax
	movl $(1024 * 1024 * 0 + PAGE_FLAGS),  (%eax)
	movl $0,							  4(%eax)
	movl $(1024 * 1024 * 2 + PAGE_FLAGS), 8(%eax)
	movl $0,							 12(%eax)
	movl $(1024 * 1024 * 4 + PAGE_FLAGS),16(%eax)
	movl $0,							 20(%eax)
	movl $(1024 * 1024 * 6 + PAGE_FLAGS),24(%eax)
	movl $0,							 28(%eax)
	movl $(1024 * 1024 * 8 + PAGE_FLAGS),32(%eax)
	movl $0,							 36(%eax)
	movl $(1024 * 1024 * 10 + PAGE_FLAGS),40(%eax)
	movl $0,							 44(%eax)
	movl $(1024 * 1024 * 12 + PAGE_FLAGS),48(%eax)
	movl $0,							 52(%eax)
	movl $(1024 * 1024 * 14 + PAGE_FLAGS),56(%eax)
	movl $0,							 60(%eax)
	movl $(1024 * 1024 * 16 + PAGE_FLAGS),64(%eax)
	movl $0,							 68(%eax)
	movl $(1024 * 1024 * 18 + PAGE_FLAGS),72(%eax)
	movl $0,							 76(%eax)
	/* Fill upper values with 0, we are still in 32 bit */

	/* Setup PDPTE */
	orl $3, %eax 			/* Set PDE to present and writeable */
	movl $PDPTE_ADDRESS, %ebx
	movl %eax, (%ebx)		/* Move address of PDE into first entry */
	movl $0, 4(%ebx)		/* Set upper 4 bytes blank */

	/* Setup PML4 */
	orl $3, %ebx 			/* Set PDPTE to present and writeable */
	movl $PML4_ADDRESS, %eax
	movl %ebx,  (%eax)
	movl $0,   4(%eax)
	movl %ebx, 8(%eax)
	movl $0,  12(%eax)

	/* Load Pagetable */
	movl %eax, %cr3

	/* Next step is to enable 64 bit mode */
	movl %cr4, %eax
	orl $0x000000B0, %eax /* PGE, PAE and PSE */
	movl %eax, %cr4

	/* Enable Long Mode & Syscall / Sysret */
	movl $0xC0000080, %ecx
	rdmsr
	orl  $0x00000101, %eax       /* Active long mode */
	wrmsr

	/* Third step is to enable paging */
	movl %cr0, %eax
	or $0x80000000, %eax
	movl %eax, %cr0

	/* Last is to load 64 bit GDT */
	lgdt (GDTR64)

	/* far jump to 64 bit mode */
	ljmp $0x8, $2f

4:
	/* Oh dang it, somethings wrong :( */
	hlt

2:
.code64
.extern init
.extern init_stack
	/* 64 Bit mode! */

	/* Clear RSP */
	movl %esp, %eax
	xor  %rsp, %rsp
	movl %eax, %esp

	movq $init_stack, %rax
	movabs $StackStart, %rbx
	movq %rbx, (%rax)

	/* Restore multiboot info */
	popq %rbx
	popq %rax

	/* Setup new stack */
	movabs $StackStart, %rcx
	movq %rcx, %rsp
	movq %rcx, %rbp

	/* Call entry point, move args into registers */
	movq %rax, %rdi
	movq %rbx, %rsi
	callq init

	/* EOK */
5:
	hlt
	jmp 5b

/* Reserve a stack for kernel */
.align 16
StackEnd:
	.fill 0x4000, 0x1, 0x0
StackStart:

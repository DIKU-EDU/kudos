/*
 * System calls.
 */
#include <cswitch.h>
#include "proc/syscall.h"
#include "kernel/halt.h"
#include "kernel/panic.h"
#include "lib/libc.h"
#include "kernel/assert.h"
#include <idt.h>
#include <irq.h>
#include <gdt.h>

/* The assembly handler */
extern void syscall_irq_handler(void);

/* Common Entry point */
extern uintptr_t syscall_entry(uintptr_t syscall, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2);


/**
 * Installs the Syscall Handler
 */
void syscall_init(void)
{
    /* Install interrupt at vector 0x80 */
    idt_install_gate(0x80, IDT_DESC_PRESENT | IDT_DESC_BIT32 | IDT_DESC_RING3, 
        (GDT_KERNEL_CODE << 3), (int_handler_t)syscall_irq_handler);
}

/**
 * Handle system calls. Interrupts are enabled when this function is
 * called.
 *
 * @param user_context The userland context (CPU registers as they
 * where when system call instruction was called in userland)
 */
void syscall_handle(regs_t *registers)
{
    /* When a syscall is executed in userland, register rdi contains
     * the number of the syscall. Registers rsi, rdx and rcx contain the
     * arguments of the syscall. The userland code expects that after
     * returning from the syscall instruction the return value of the
     * syscall is found in register rax. Before entering this function
     * the userland context has been saved to user_context and after
     * returning from this function the userland context will be
     * restored from user_context.
     */
     registers->rax = syscall_entry(registers->rdi, registers->rsi, 
                                    registers->rdx, registers->rcx);
}

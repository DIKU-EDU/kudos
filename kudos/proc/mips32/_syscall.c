/*
 * System calls.
 */
#include <cswitch.h>
#include "proc/syscall.h"
#include "kernel/halt.h"
#include "kernel/panic.h"
#include "lib/libc.h"
#include "kernel/assert.h"

/* Common Entry point */
extern uintptr_t syscall_entry(
  uintptr_t syscall, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2);

/**
 * Handle system calls. Interrupts are enabled when this function is
 * called.
 *
 * @param user_context The userland context (CPU registers as they
 * where when system call instruction was called in userland)
 */
void syscall_handle(context_t *user_context)
{
    /* When a syscall is executed in userland, register a0 contains
     * the number of the syscall. Registers a1, a2 and a3 contain the
     * arguments of the syscall. The userland code expects that after
     * returning from the syscall instruction the return value of the
     * syscall is found in register v0. Before entering this function
     * the userland context has been saved to user_context and after
     * returning from this function the userland context will be
     * restored from user_context.
     */
    user_context->cpu_regs[MIPS_REGISTER_V0] =
        syscall_entry(user_context->cpu_regs[MIPS_REGISTER_A0],
            user_context->cpu_regs[MIPS_REGISTER_A1],
            user_context->cpu_regs[MIPS_REGISTER_A2],
            user_context->cpu_regs[MIPS_REGISTER_A3]);

    /* Move to next instruction after system call */
    user_context->pc += 4;
}

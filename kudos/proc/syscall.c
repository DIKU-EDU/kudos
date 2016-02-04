/*
 * System calls.
 */
#include <cswitch.h>
#include "proc/syscall.h"
#include "kernel/halt.h"
#include "kernel/panic.h"
#include "lib/libc.h"
#include "kernel/assert.h"
#include "vm/memory.h"

/**
 * Handle system calls. Interrupts are enabled when this function is
 * called.
 *
 * @param user_context The userland context (CPU registers as they
 * where when system call instruction was called in userland)
 */
uintptr_t syscall_entry(uintptr_t syscall, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2)
{
  arg0 = arg0;
  arg1 = arg1;
  arg2 = arg2;
  /* When a syscall is executed in userland, register a0 contains
   * the number of the syscall. Registers a1, a2 and a3 contain the
   * arguments of the syscall. The userland code expects that after
   * returning from the syscall instruction the return value of the
   * syscall is found in register v0. Before entering this function
   * the userland context has been saved to user_context and after
   * returning from this function the userland context will be
   * restored from user_context.
   */
  switch(syscall)
    {
    case SYSCALL_HALT:
      halt_kernel();
      break;
    default:
      KERNEL_PANIC("Unhandled system call\n");
    }

  /* No return code / value if we reach here */
  return 0;
}

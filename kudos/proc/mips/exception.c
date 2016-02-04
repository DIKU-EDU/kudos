/*
 * Exception handling.
 */

#include "kernel/panic.h"
#include "kernel/interrupt.h"
#include "lib/libc.h"
#include "kernel/thread.h"
#include <exception.h>

void syscall_handle(context_t *user_context);

/** Handles an exception (code != 0) that occured in user mode. Will
 * call appropiate handlers for the exception or panic if the
 * exception should not have occured or does not (yet) have a handler.
 * Interrupts are disabled by EXL when this function is called, and
 * must be so when this fucntion returns. Interrupts may be enabled
 * during execution of this function if needed.
 *
 * @param exception The exception code field from the CP0 Cause register
 */
void user_exception_handle(int exception)
{
  thread_table_t *my_entry;

  /* While interrupts are disabled here, they can be enabled when
     handling system calls and certain other exceptions if needed.
     For normal TLB exceptions it is not desirable that context is
     switched before TLB is filled. */
  _interrupt_disable();

  /* Clear EXL to make normal interrupt disable/enable work. */
  _interrupt_clear_EXL();

  /* Save usermode context to user_context for later reference in syscalls */
  my_entry= thread_get_current_thread_entry();
  my_entry->user_context = my_entry->context;

  switch(exception) {
  case EXCEPTION_TLBM:
    KERNEL_PANIC("TLB Modification: not handled yet");
    break;
  case EXCEPTION_TLBL:
    KERNEL_PANIC("TLB Load: not handled yet");
    break;
  case EXCEPTION_TLBS:
    KERNEL_PANIC("TLB Store: not handled yet");
    break;
  case EXCEPTION_ADDRL:
    KERNEL_PANIC("Address Error Load: not handled yet");
    break;
  case EXCEPTION_ADDRS:
    KERNEL_PANIC("Address Error Store: not handled yet");
    break;
  case EXCEPTION_BUSI:
    KERNEL_PANIC("Bus Error Instruction: not handled yet");
    break;
  case EXCEPTION_BUSD:
    KERNEL_PANIC("Bus Error Data: not handled yet");
    break;
  case EXCEPTION_SYSCALL:
    _interrupt_enable();
    syscall_handle(my_entry->user_context);
    _interrupt_disable();
    break;
  case EXCEPTION_BREAK:
    KERNEL_PANIC("Breakpoint: not handled yet");
    break;
  case EXCEPTION_RESVI:
    KERNEL_PANIC("Reserved instruction: not handled yet");
    break;
  case EXCEPTION_COPROC:
    KERNEL_PANIC("Coprocessor unusable: buggy assembler code?");
    break;
  case EXCEPTION_AOFLOW:
    KERNEL_PANIC("Arithmetic overflow: buggy assembler code?");
    break;
  case EXCEPTION_TRAP:
    KERNEL_PANIC("Trap: this just should not happen");
    break;
  default:
    KERNEL_PANIC("Unknown exception");
  }

  /* Interrupts are disabled by setting EXL after this point. */
  _interrupt_set_EXL();
  _interrupt_enable();

}

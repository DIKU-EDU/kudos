/*
 * Context switch.
 */

#include <cswitch.h>
#include "kernel/interrupt.h"
#include <gdt.h>
#include "kernel/config.h"
#include "vm/memory.h"
#include "lib/libc.h"
#include "kernel/scheduler.h"
#include "kernel/thread.h"
#include <tss.h>

#define THREAD_FLAGS    0x200202

void _context_init(context_t *cxt, virtaddr_t entry, virtaddr_t endentry,
                   virtaddr_t stack, uint32_t args)
{
  /* Setup a stack */
  if(stack & 0xF)
    {
      stack = (stack & 0xFFFFFFFFFFFFFFF0);
      stack += 0x10;
    }

  virtaddr_t *rsp = (virtaddr_t*)stack;
  virtaddr_t rbp = stack;

  /* Call stack, start at end-entry */
  *(rsp--) = endentry;
  *(rsp--) = (uint64_t)(GDT_KERNEL_DATA << 3);
  *(rsp--) = rbp;
  *(rsp--) = (uint64_t)THREAD_FLAGS;
  *(rsp--) = (uint64_t)(GDT_KERNEL_CODE << 3);
  *(rsp--) = entry;

  /* Reset rbp */
  rbp = (uint64_t)rsp;

  /* Initialise Registers */
  *(rsp--) = 0; //R15
  *(rsp--) = 0; //R14
  *(rsp--) = 0; //R13
  *(rsp--) = 0; //R12
  *(rsp--) = 0; //R11
  *(rsp--) = 0; //R10
  *(rsp--) = 0; //R9
  *(rsp--) = 0; //R8
  *(rsp--) = (uint64_t)args;    //RDI
  *(rsp--) = 0; //RSI
  *(rsp--) = rbp;       //RBP
  *(rsp--) = 0; //RSP
  *(rsp--) = 0; //RBX
  *(rsp--) = 0; //RDX
  *(rsp--) = 0; //RCX
  *(rsp) = 0;           //RAX

  /* Set stack */
  cxt->stack = rsp;

  /* Setup memory */
  cxt->pml4 = (uint64_t)vmm_get_kernel_pml4();
  cxt->virt_memory = vmm_get_kernel_pml4();
  cxt->flags = 0;
}

void _context_enter_userland(context_t *cxt)
{
  /* Setup a stack */
  uintptr_t stack = ((uintptr_t)cxt->stack) - 0x10;
  if(stack & 0xF)
    {
      stack = (stack & 0xFFFFFFFFFFFFFFF0);
      stack += 0x10;
    }

  virtaddr_t *rsp = (virtaddr_t*)stack;
  virtaddr_t rbp = stack;

  /* Call stack, start at end-entry */
  *(rsp--) = 0;
  *(rsp--) = (uint64_t)(GDT_KERNEL_DATA << 3);
  *(rsp--) = rbp;
  *(rsp--) = (uint64_t)THREAD_FLAGS;
  *(rsp--) = (uint64_t)(GDT_KERNEL_CODE << 3);
  *(rsp--) = cxt->rip;

  /* Reset rbp */
  rbp = (uint64_t)rsp;

  /* Initialise Registers */
  *(rsp--) = 0; //R15
  *(rsp--) = 0; //R14
  *(rsp--) = 0; //R13
  *(rsp--) = 0; //R12
  *(rsp--) = 0; //R11
  *(rsp--) = 0; //R10
  *(rsp--) = 0; //R9
  *(rsp--) = 0; //R8
  *(rsp--) = 0; //RDI
  *(rsp--) = 0; //RSI
  *(rsp--) = rbp;       //RBP
  *(rsp--) = 0; //RSP
  *(rsp--) = 0; //RBX
  *(rsp--) = 0; //RDX
  *(rsp--) = 0; //RCX
  *(rsp) = 0;           //RAX

  /* Update stack */
  cxt->stack = rsp;

  /* Enable user-mode */
  thread_get_current_thread_entry()->user_context = cxt;
  thread_get_current_thread_entry()->attribs |= THREAD_FLAG_ENTERUSER;

  /* Yield, like NOW */
  thread_switch();

  /* No escape from this path */
  for(;;);
}

void _context_set_ip(context_t *cxt, virtaddr_t ip)
{
  /* Set EIP */
  cxt->rip = ip;
}

void _context_enable_ints(context_t *cxt)
{
  /* Modify status */
  cxt->flags |= EFLAGS_INTERRUPT_FLAG;

  /* Setup memory */
  cxt->pml4 = (uint64_t)vmm_get_kernel_pml4();
  cxt->virt_memory = vmm_get_kernel_pml4();
  cxt->flags = 0;
}

void _context_set_sp(context_t *cxt, virtaddr_t sp)
{
  cxt->stack = (virtaddr_t*)sp;
}

struct dirty_dirty_hack {
    uint64_t stack;
    uint64_t pml4;
};

struct dirty_dirty_hack task_switch(uint64_t *stack)
{
  /* OK, We want to save current stack */
  thread_table_t *task = thread_get_current_thread_entry();
  virtaddr_t new_stack;

  /* Is it a usertask?  */
  if(task->attribs & THREAD_FLAG_USERMODE)
    task->user_context->stack = stack;
  else
    task->context->stack = stack;

  /* Schedule */
  scheduler_schedule();

  /* Get new task */
  task = thread_get_current_thread_entry();

  /* Update TSS */
  tss_setstack(0, (uint64_t)task->context->stack);

  /* Test if this new task is set to
   * enter usermode */
  if(task->attribs & THREAD_FLAG_ENTERUSER)
    {
      task->attribs &= ~THREAD_FLAG_ENTERUSER;
      task->attribs |= THREAD_FLAG_USERMODE;
    }

  /* return new stack */
  if(task->attribs & THREAD_FLAG_USERMODE)
    new_stack = task->user_context->stack;
  else
    new_stack = task->context->stack;

  //return new_stack;
  struct dirty_dirty_hack tmp;
  tmp.stack = new_stack;              //RAX
  tmp.pml4 = task->context->pml4;     //RDX
  return tmp;
}

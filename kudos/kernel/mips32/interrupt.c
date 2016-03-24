/*
 * Context switch.
 */

#include "lib/types.h"
#include "kernel/config.h"
#include "kernel/stalloc.h"
#include "kernel/panic.h"
#include "kernel/scheduler.h"
#include "kernel/interrupt.h"
#include "drivers/polltty.h"
#include "kernel/thread.h"
#include "lib/libc.h"
#include <tlb.h>

/* Interrupt vector addresses (only these three should be ever used) */
#define INTERRUPT_VECTOR_ADDRESS1 0x80000000
#define INTERRUPT_VECTOR_ADDRESS2 0x80000180
#define INTERRUPT_VECTOR_ADDRESS3 0x80000200
#define INTERRUPT_VECTOR_LENGTH  8

extern TID_t scheduler_current_thread[CONFIG_MAX_CPUS];

/* Pointers to interrupt stack for each processor */
uint32_t interrupt_stacks[CONFIG_MAX_CPUS];

/* Table for the registered interrupt handlers */
static interrupt_entry_t interrupt_handlers[CONFIG_MAX_DEVICES];


/** Initializes interrupt handling. Allocates interrupt stacks for
 * each processor, initializes the interrupt vectors and initializes
 * the registered interrupt handler table.
 *
 * @param num_cpus Number of CPUs in the system
 */
void interrupt_init(int num_cpus) {
  int i;
  uint32_t *iv_area1 = (uint32_t *)INTERRUPT_VECTOR_ADDRESS1;
  uint32_t *iv_area2 = (uint32_t *)INTERRUPT_VECTOR_ADDRESS2;
  uint32_t *iv_area3 = (uint32_t *)INTERRUPT_VECTOR_ADDRESS3;
  uint32_t ret;

  if (num_cpus < 1 || num_cpus > CONFIG_MAX_CPUS)
    KERNEL_PANIC("Too few or many CPUs found");

  /* Allocate interrupt stacks for each processor */
  for(i = 0; i < num_cpus; i++) {
    ret = (uint32_t)stalloc(PAGE_SIZE);
    if (ret == 0)
      KERNEL_PANIC("Unable to allocate interrupt stacks");
    interrupt_stacks[i] = ret+PAGE_SIZE-4;
  }

  /* Copy the interrupt vector code to its positions.All vectors
   * will contain the same code.
   */
  for(i = 0 ; i < INTERRUPT_VECTOR_LENGTH ; i++) {
    iv_area1[i] = ((uint32_t *) (uintptr_t) &_cswitch_vector_code)[i];
    iv_area2[i] = ((uint32_t *) (uintptr_t) &_cswitch_vector_code)[i];
    iv_area3[i] = ((uint32_t *) (uintptr_t) &_cswitch_vector_code)[i];
  }

  /* Initialize the handler table to empty */
  for (i=0; i<CONFIG_MAX_DEVICES; i++) {
    interrupt_handlers[i].device = NULL;
    interrupt_handlers[i].irq = 0;
    interrupt_handlers[i].handler = NULL;
  }
}


/** Registers an interrupt handler for one or more interrupts
 * (IRQs). When registered, a \texttt{handler(device)} function call
 * will be made if any of the interrupts in \texttt{irq} occured.
 *
 * @param irq Mask of interrupts this handler wants to handle
 * @param handler The interrupt handling function
 * @param device The device registered for the interrupt, will be
 * given as a parameter for handler
 */
void interrupt_register(uint32_t irq,
                        int_handler_t handler,
                        device_t *device)
{
  int i = 0;

  /* Check that IRQ mask is sane */
  if ((irq & ~(uint32_t)INTERRUPT_MASK_ALL)!= 0) {
    kprintf("Unsupported IRQ mask:%.8x\n", irq);
    KERNEL_PANIC("interrupt_register");
  }

  /* No need for spinlock, this should not be called after other CPUs
   * are enabled.
   */

  while (interrupt_handlers[i].device != NULL && i < CONFIG_MAX_DEVICES) i++;

  if (i >= CONFIG_MAX_DEVICES)
    KERNEL_PANIC("Interrupt handler table is full");

  interrupt_handlers[i].device = device;
  interrupt_handlers[i].irq = irq;
  interrupt_handlers[i].handler = handler;
}


/** Handles an interrupt (exception code 0). All interrupt handlers
 * that are registered for any of the occured interrupts (hardware
 * 0-5, software 0-1) are called. The scheduler is called if a timer
 * interrupt (hardware 5) or a context switch request (software
 * interrupt 0) occured, or if the currently running thread for the
 * processor is the idle thread.
 *
 * @param cause The Cause register from CP0
 */
void interrupt_handle(virtaddr_t cause) {
  int this_cpu, i;

  if(cause & INTERRUPT_CAUSE_SOFTWARE_0) {
    _interrupt_clear_sw0();
  }

  this_cpu = _interrupt_getcpu();

  /* Exceptions should be handled elsewhere: */
  if((cause  & 0x0000007c) != 0) {
    kprintf("Caught exception, cause %.8x, CPU %i\n", cause, this_cpu);
    KERNEL_PANIC("Exception in interrupt_handle");
  }


  /* Call appropiate interrupt handlers.  Handlers cannot be
   * unregistered, so after the first empty * entry all others are
   * also empty.
   */
  for (i=0; i<CONFIG_MAX_DEVICES; i++) {
    if (interrupt_handlers[i].device == NULL)
      break;

    /* If this handler is registered for any of the interrupts
     * that occured, call it.
     */
    if ((cause & interrupt_handlers[i].irq) != 0)
      interrupt_handlers[i].handler(interrupt_handlers[i].device);
  }


  /* Timer interrupt (HW5) or requested context switch (SW0)
   * Also call scheduler if we're running the idle thread.
   */
  if((cause & (INTERRUPT_CAUSE_SOFTWARE_0 |
               INTERRUPT_CAUSE_HARDWARE_5)) ||
     scheduler_current_thread[this_cpu] == IDLE_THREAD_TID) {
    scheduler_schedule();

    /* Until we have proper VM we must manually fill
       the TLB with pagetable entries before running code using
       given pagetable. Note that this method limits pagetable
       rows (possible mapping pairs) to 16 and can't be used
       with proper pagetables and VM.

       Note that if you remove this call (which you probably do when
       you implement proper VM), you must manually call _tlb_set_asid
       here. See the implementation of tlb_fill on details how to do that.
    */
    tlb_fill(thread_get_current_thread_entry()->pagetable);
  }
}

interrupt_status_t _interrupt_is_disabled(void)
{
  interrupt_status_t intr_state = _interrupt_get_state();
  if(!(intr_state & INTERRUPT_MASK_ALL)
     || !(intr_state & INTERRUPT_MASK_MASTER))
    return 1;
  else
    return 0;
}

/*
 * Process platform specific code.
 */
#include <arch.h>
#include "proc/process.h"
#include "kernel/thread.h"
#include "kernel/assert.h"
#include "kernel/interrupt.h"
#include "kernel/config.h"
#include "vm/memory.h"

void process_set_pagetable(pagetable_t *pagetable)
{
  interrupt_status_t intr_status;

  /* Put the mapped pages into TLB. Here we assume that the pages fit
     into the TLB. After writing proper TLB exception handling this
     call should be skipped, leaving the function empty. */
  intr_status = _interrupt_disable();
  tlb_fill(pagetable);
  _interrupt_set_state(intr_status);
}

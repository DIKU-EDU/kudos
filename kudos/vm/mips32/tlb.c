/*
 * TLB handling
 */

#include <stdbool.h>

#include "kernel/panic.h"
#include "kernel/assert.h"
#include <pagetable.h>
#include <tlb.h>
#include <types.h>

void tlb_modified_exception(UNUSED bool in_userland)
{
  KERNEL_PANIC("Unhandled TLB modified exception");
}

void tlb_load_exception(UNUSED bool in_userland)
{
  KERNEL_PANIC("Unhandled TLB load exception");
}

void tlb_store_exception(UNUSED bool in_userland)
{
  KERNEL_PANIC("Unhandled TLB store exception");
}

/**
 * Fill TLB with given pagetable. This function is used to set memory
 * mappings in CP0's TLB before we have a proper TLB handling system.
 * This approach limits the maximum mapping size to 128kB.
 *
 * @param pagetable Mappings to write to TLB.
 *
 */

void tlb_fill(pagetable_t *pagetable)
{
  if(pagetable == NULL)
    return;

  /* Check that the pagetable can fit into TLB. This is needed until
     we have proper VM system, because the whole pagetable must fit
     into TLB. */
  KERNEL_ASSERT(pagetable->valid_count <= (_tlb_get_maxindex()+1));

  _tlb_write(pagetable->entries, 0, pagetable->valid_count);

  /* Set ASID field in Co-Processor 0 to match thread ID so that
     only entries with the ASID of the current thread will match in
     the TLB hardware. */
  _tlb_set_asid(pagetable->ASID);
}

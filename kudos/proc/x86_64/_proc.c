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
#include "lib/libc.h"

void process_set_pagetable(pagetable_t *pagetable)
{
    /* Transform the pagetable_t to pml4 */
    interrupt_status_t intr_status;
    pml4_t *pml4 = (pml4_t*)pagetable;

    /* Switch page-dir */
    /* This means we can make use of the above mappings in
     * a seperate memory region, this means we are still operating
     * in ring 0 and everything is kernel like, except we dont have
     * the heap mapped in, and we now have some extra pages mapped in
     * if vm_map worked correctly hihi */
    intr_status = _interrupt_disable(); //
    vmm_setcr3((uintptr_t)pml4);
    thread_get_current_thread_entry()->context->pml4 = (uintptr_t)pml4;
    thread_get_current_thread_entry()->context->virt_memory = pml4;
    _interrupt_set_state(intr_status);
}

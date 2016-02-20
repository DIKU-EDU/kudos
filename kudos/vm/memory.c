/*
 * Copying into virtual memory space.
 */

#include <pagetable.h>
#include <arch.h>
#include "vm/memory.h"
#include "kernel/assert.h"
#include "lib/libc.h"

void vm_memwrite(pagetable_t *pagetable, unsigned int buflen,
                 virtaddr_t target, const void *void_source)
{
  const uint8_t *source = (const uint8_t*)void_source;

  while (buflen > 0) {
    /* Find offset in target virtual page. */
    virtaddr_t voffset = target & PAGE_OFFSET_MASK;

    /* Find physical page corresponding to target virtual page. */
    physaddr_t fpage = vm_getmap(pagetable, target);
    KERNEL_ASSERT(fpage != 0);

    /* Find physical address corresponding to virtual address. */
    physaddr_t faddr = fpage + voffset;

    /* Copy as much as possible into the physical page. */
    int to_copy = MIN(buflen, PAGE_SIZE - voffset);
    memcopy(to_copy, (void*)ADDR_PHYS_TO_KERNEL(faddr), source);

    /* Subtract amount copied from buflen, move source ahead, and try
       again. */
    source += to_copy;
    buflen -= to_copy;
  }
}

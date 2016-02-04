/*
 * Kernel Heap Memory Management
 */

#include "vm/memory.h"
#include "kernel/spinlock.h"
#include "kernel/interrupt.h"
#include "kernel/assert.h"
#include "lib/libc.h"

/* Globals, we keep track of our heap */
virtaddr_t heap_salloc = MM_HEAP_LOCATION;
virtaddr_t heap_next_salloc = MM_HEAP_LOCATION;
spinlock_t heap_lock = 0;
heap_header_t *heap;

/* Internal allocation for structures */
virtaddr_t *salloc(uint32_t size)
{
  /* Lock us down */
  virtaddr_t retaddr = 0;
  interrupt_status_t intr_status = _interrupt_disable();
  spinlock_acquire(&heap_lock);

  /* Make sure we have enough space mapped */
  if((heap_salloc + size) >= heap_next_salloc)
    {
      /* Map us in, in kernel directory */
      vm_map(0, physmem_allocblock(), heap_next_salloc, 0);

      /* Increase max */
      heap_next_salloc += PAGE_SIZE;
    }

  /* Return address */
  retaddr = heap_salloc;
  heap_salloc += size;

  /* Done */
  spinlock_release(&heap_lock);
  _interrupt_set_state(intr_status);

  /* Return */
  return (virtaddr_t*)retaddr;
}

void heap_init(void)
{
  /* Setup */
  spinlock_reset(&heap_lock);

  /* We setup ONE block with the heap size,
   * it will split it each time for the needed allocation
   * */
  heap = (heap_header_t*)salloc(sizeof(heap_header_t));
  heap->addr = MM_HEAP_LOCATION + MM_HEAP_ADDR_OFFSET; //Start of heap
  heap->length = MM_HEAP_SIZE;
  heap->flags = 0;
  heap->link = 0;

  /* Map first to spare a page-fault */
  vm_map(0, physmem_allocblock(), heap->addr, 0);
}

virtaddr_t *kmalloc(uint32_t length)
{
  /* unused */
  heap_header_t *itr = heap, *prev = 0;
  virtaddr_t addr = 0;

  /* 16 Byte align */
  if(length & 0xF)
    {
      length = (length & 0xFFFFFFF0);
      length += 0x10;
    }

  /* We go through the list and find an allocated block that
   * has enough space, when we find one, we split it and insert it
   * before the splitted block in the list */
  while(itr)
    {
      /* Is it free? */
      if(!(itr->flags & MM_HEAP_FLAG_ALLOCATED))
        {
          /* No.. */
          /* Does it fit? */
          if(itr->length > length)
            {
              /* Yes, split it */

              /* Step 1. Create a new block at the address */
              heap_header_t *block =
                (heap_header_t*)salloc(sizeof(heap_header_t));
              block->addr = itr->addr;
              block->length = length;
              block->flags = MM_HEAP_FLAG_ALLOCATED;
              block->link = itr;

              /* Step 2. Modify the original block */
              itr->addr += length;
              itr->length -= length;

              /* Step 3. Fix the list order */
              if(prev != 0)
                prev->link = block;
              else
                heap = block;

              /* done, break */
              addr = block->addr;
              break;
            }
        }

      /* Get next node */
      prev = itr;
      itr = itr->link;
    }

  /* Sanity */
  KERNEL_ASSERT(addr != 0);

  return (virtaddr_t*)addr;
}


void kfree(void *ptr)
{
  ptr = ptr;
  //Left as an exercise
}

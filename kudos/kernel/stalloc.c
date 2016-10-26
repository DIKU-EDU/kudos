/*
 * Kernel memory allocation.
 */

#include "lib/types.h"
#include "lib/libc.h"
#include "drivers/device.h"
#include "kernel/stalloc.h"
#include "kernel/panic.h"

/** @name Permanent kernel memory allocation
 *
 * This module implements permanent kernel memory allocation in
 * unmapped memory. The memory is allocated after the kernel image.
 * The memory allocated through this module cannot be freed.
 *
 * @{
 */

/* Special symbol, which is put to the end of the kernel binary by the
   linker */
extern uint64_t KERNEL_ENDS_HERE;

/**  The start of the free area. */
static physaddr_t free_area_start;
physaddr_t stalloced_total;


/**
 * Disable static memory allocation for kernel. This is called from
 * the virtual memory initialization function.
 */
void stalloc_disable()
{
  stalloced_total = free_area_start-((physaddr_t)&KERNEL_ENDS_HERE);
  free_area_start = 0xffffffff;
}

/**
 * Initializes the variables used by stalloc. Searches for the MemInfo
 * device descriptor to find out the memory size. Sets the
 * \texttt{free_area_start} to contain the address that comes right
 * after the kernel image. Should not be called before polling TTY is
 * initialized. Causes kernel panic if the MemInfo device descriptor
 * is not found.
 */
void stalloc_init(void)
{
  free_area_start = (physaddr_t) &KERNEL_ENDS_HERE;
  stalloced_total = 0;

  if (free_area_start & 0x03) {
    free_area_start += 4;
    free_area_start &= 0xfffffffc;
  }
}

/**
 * Allocates permanent memory for the kernel in unmapped memory. Call
 * of this function after virtual memory has been initialized will
 * cause kernel panic. Panics if memory can't be allocated.
 *
 * @param bytes The number of bytes to be allocated.
 *
 * @return The start address of the reseved memory address.
 */
physaddr_t *stalloc(int bytes)
{
  physaddr_t res;

  /* Panic if VM is initialized */
  if (free_area_start == 0xffffffff){
    KERNEL_PANIC("Attempting to use stalloc after vm init\n");
  }

  if (free_area_start == 0) {
    KERNEL_PANIC("Attempting to use stalloc before initialization\n");
  }    

  /* bytes == 0 may be useful for aligning memory so it is allowed */
  if (bytes < 0)
    KERNEL_PANIC("Attempting to stalloc negative amount of bytes\n");

  res = free_area_start;

  free_area_start += bytes;

  /* Check that the start of free area is aligned on a word
     boundary */
  if (free_area_start & 0x03) {
    free_area_start += 4;
    free_area_start &= 0xfffffffc;
  }

  return (physaddr_t*)res;
}


/** @} */

/*
 * Physical Memory Management
 */

#include "vm/memory.h"
#include "lib/bitmap.h"
#include "kernel/stalloc.h"
#include "kernel/spinlock.h"
#include "kernel/interrupt.h"
#include "kernel/assert.h"

/** @name Page pool
 *
 * Functions and data structures for handling physical page reservation.
 *
 * @{
 */

/* Bitmap field of physical pages. Length is number of physical pages
   rounded up to a word boundary */
static bitmap_t *physmem_free_pages;

/* Number of physical pages */
static int physmem_num_pages;

/* Number of free physical pages */
static int physmem_num_free_pages;

/* Number of last staticly reserved page. This is needed to ensure
   that staticly reserved pages are not freed in accident (or in
   purpose).  */
static int physmem_static_end;

/* Spinlock to handle synchronous access to physmem_free_pages */
static spinlock_t physmem_slock;

/**
 * Returns the number of memory pages present in the system. Causes
 * kernel panic if the MemInfo device is not found.
 *
 * @return The number of memory pages
 */
int physmem_get_size()
{
  uint32_t num_pages = 0;

  int i;

  io_descriptor_t *io_desc;

  io_desc = (io_descriptor_t *)IO_DESCRIPTOR_AREA;

  /* Find MemInfo meta device */
  for(i = 0; i < YAMS_MAX_DEVICES; i++) {
    if (io_desc->type == 0x101) {
      num_pages = (*(uint32_t *)io_desc->io_area_base);
      break;
    }
    io_desc++;
  }

  if (num_pages == 0)
    KERNEL_PANIC("No MemInfo device found.");

  return num_pages;
}

/**
 * Returns the number of pages statically reserved for the kernel.
 *
 * @return The number of pages
 */
int physmem_get_reserved_size()
{
  int num_res_pages;
  uint32_t system_memory_size = 0;
  physaddr_t free_start = (physaddr_t)stalloc(1);
  system_memory_size = physmem_get_size() * PAGE_SIZE;

  num_res_pages = (free_start - 0x80000000) / PAGE_SIZE;

  if (((free_start - 0x80000000) % PAGE_SIZE) != 0)
    num_res_pages++;

  kprintf("Kernel size is 0x%.8x (%d) bytes\n",
          (free_start - KERNEL_BOOT_ADDRESS),
          (free_start - KERNEL_BOOT_ADDRESS));
  kprintf("System Memory Size: 0x%x bytes\n", system_memory_size);

  return num_res_pages;
}


/**
 * Physical memory initialization. Finds out number of physical pages and
 * number of staticly reserved physical pages. Marks reserved pages
 * reserved in physmem_free_pages.
 */
void physmem_init(void *bootinfo)
{
  int num_res_pages;
  int i;

  /* We dont use this */
  bootinfo = bootinfo;

  physmem_num_pages = physmem_get_size();

  physmem_free_pages =
    (uint32_t *)stalloc(bitmap_sizeof(physmem_num_pages));
  bitmap_init(physmem_free_pages, physmem_num_pages);

  /* Note that number of reserved pages must be get after we have
     (staticly) reserved memory for bitmap. */
  num_res_pages = physmem_get_reserved_size();
  physmem_num_free_pages = physmem_num_pages - num_res_pages;
  physmem_static_end = num_res_pages;

  for (i = 0; i < num_res_pages; i++)
    bitmap_set(physmem_free_pages, i, 1);

  spinlock_reset(&physmem_slock);

  kprintf("Physmem: Found %d pages of size %d\n", physmem_num_pages,
          PAGE_SIZE);
  kprintf("Physmem: Static allocation for kernel: %d pages\n",
          num_res_pages);
}

/**
 * Finds first free physical page and marks it reserved.
 *
 * @return Address of first free physical page, zero if no free pages
 * are available.
 */
physaddr_t physmem_allocblock(void)
{
  interrupt_status_t intr_status;
  int i;

  intr_status = _interrupt_disable();
  spinlock_acquire(&physmem_slock);

  if (physmem_num_free_pages > 0) {
    i = bitmap_findnset(physmem_free_pages,physmem_num_pages);
    physmem_num_free_pages--;

    /* There should have been a free page. Check that the physmem
       internal variables are in synch. */
    KERNEL_ASSERT(i >= 0 && physmem_num_free_pages >= 0);
  } else {
    i = 0;
  }

  spinlock_release(&physmem_slock);
  _interrupt_set_state(intr_status);
  return i*PAGE_SIZE;
}

/**
 * Frees given page. Given page should be reserved, but not staticly
 * reserved.
 *
 * @param phys_addr Page to be freed.
 */
void physmem_freeblock(void *Ptr)
{
  interrupt_status_t intr_status;
  int i;

  physaddr_t phys_addr = (physaddr_t)Ptr;
  i = phys_addr / PAGE_SIZE;

  /* A page allocated by stalloc should not be freed. */
  KERNEL_ASSERT(i >= physmem_static_end);

  intr_status = _interrupt_disable();
  spinlock_acquire(&physmem_slock);

  /* Check that the page was reserved. */
  KERNEL_ASSERT(bitmap_get(physmem_free_pages, i) == 1);

  bitmap_set(physmem_free_pages, i, 0);
  physmem_num_free_pages++;

  spinlock_release(&physmem_slock);
  _interrupt_set_state(intr_status);
}



/** @} */

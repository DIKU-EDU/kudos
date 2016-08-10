/*
 * Physical Memory Manager for KUDOS
 */

#include <multiboot.h>
#include <arch.h>
#include "vm/memory.h"
#include "lib/libc.h"
#include "kernel/stalloc.h"
#include "kernel/spinlock.h"
#include "kernel/interrupt.h"
#include "kernel/panic.h"

/* PMM Defines */
#define PMM_BLOCKS_PER_BYTE 0x8

/* Memory Map */
uint64_t *_mem_bitmap;
uint64_t bitmap_size;
uint64_t memory_size;
uint64_t total_blocks;
uint64_t used_blocks;
uint64_t highest_page;
spinlock_t *physmem_lock;

/* Memory Bitmap Helpers */
void memmap_setbit(int64_t bit)
{
  _mem_bitmap[bit / 64] |= (1 << (bit % 64));
}

void memmap_unsetbit(int64_t bit)
{
  _mem_bitmap[bit / 64] &= ~(1 << (bit % 64));
}

int64_t memmap_testbit(int64_t bit)
{
  return _mem_bitmap[bit / 64] & (1 << (bit % 64));
}

void physmem_freeregion(uint64_t start_address, uint64_t length)
{
  int64_t Align = (int64_t)(start_address / PMM_BLOCK_SIZE);
  int64_t Blocks = (int64_t)(length / PMM_BLOCK_SIZE);
  int64_t i = (int64_t)start_address;

  /* Free Blocks */
  for(; Blocks > 0; Blocks--, i += PMM_BLOCK_SIZE)
    {
      /* Free it */
      memmap_unsetbit(Align++);
      used_blocks--;

      if(i > highest_page)
        highest_page = i;
    }

  /* Make sure first block is always used! */
  /* Allocs must never be 0 */
  memmap_setbit(0);
}

int64_t physmem_getframe()
{
  uint64_t i, j;

  /* Loop through bitmap */
  for(i = 0; i < total_blocks; i++)
    {
      if(_mem_bitmap[i] != 0xFFFFFFFFFFFFFFFF)
        {
          for(j = 0; j < 64; j++)
            {
              int64_t bit = 1 << j;

              if(!(_mem_bitmap[i] & bit))
                return (int64_t)(i * 8 * 8 + j);
            }
        }
    }

  /* End of Memory */
  return -1;
}

int64_t physmem_getframes(int64_t count)
{
  uint64_t i, j, k;

  /* Sanity */
  if(count == 0)
    return -1;

  if(count == 1)
    return physmem_getframe();

  /* Loop through bitmap */
  for(i = 0; i < total_blocks; i++)
    {
      if(_mem_bitmap[i] != 0xFFFFFFFFFFFFFFFF)
        {
          for(j = 0; j < 64; j++)
            {
              int64_t bit = 1 << j;
              if(!(_mem_bitmap[i] & bit))
                {
                  int64_t starting_bit = i * 64;
                  int64_t free = 0;
                  starting_bit += j;

                  /* Get the free bit in qword at index i */
                  for(k = 0; k < count; k++)
                    {
                      /* Test if it is free */
                      if(memmap_testbit(starting_bit + k) == 0)
                        free++;

                      /* Did we have enough free blocks? */
                      if(free == count)
                        return starting_bit;
                    }
                }
            }
        }
    }

  /* End of Memory */
  return -1;
}

void physmem_init(void *boot_info)
{
  multiboot_info_t *mb_info = (multiboot_info_t*)boot_info;
  uint64_t *mem_ptr = (uint64_t*)(uint64_t)mb_info->memory_map_addr;
  uint64_t Itr = 0, last_address = 0;

  /* Setup Memory Stuff */
  highest_page = 0;
  memory_size = mb_info->memory_high;
  memory_size += mb_info->memory_low;
  total_blocks = (memory_size * 1024) / PAGE_SIZE;
  used_blocks = total_blocks;
  bitmap_size = total_blocks / PMM_BLOCKS_PER_BYTE;
  _mem_bitmap = (uint64_t*)stalloc(bitmap_size);
  physmem_lock = (spinlock_t*)stalloc(sizeof(spinlock_t));
  spinlock_reset(physmem_lock);

  /* Set all memory as used, and use memory map to set free */
  memoryset(_mem_bitmap, 0xF, bitmap_size);

  /* Physical Page Bitmap */
  kprintf("Memory size: %u Kb\n", (uint32_t)memory_size);

  /* Go through regions */
  for(Itr = (uint64_t)mem_ptr;
      Itr < ((uint64_t)mem_ptr + mb_info->memory_map_length);
      )
    {
      /* Get next member */
      mem_region_t *mem_region = (mem_region_t*)Itr;

      /* Output */
      //kprintf("Memory Region: Address 0x%xL, length 0x%xL, Type %u\n",
      //        mem_region->base_address, mem_region->length, mem_region->Type);

      /* Is it free? */
      if(mem_region->type == MEMTYPE_FREE)
        physmem_freeregion(mem_region->base_address,
                           mem_region->length);

      /* Advance by one structure */
      Itr += sizeof(mem_region_t);
    }

  /* Mark all memory up to the static allocation point as used */
  last_address = (physaddr_t)stalloc(1);
  stalloc_disable();

  for(Itr = physmem_allocblock(); Itr < last_address;)
    Itr = physmem_allocblock();

  /* Debug*/
  kprintf("New memory allocation starts at 0x%xl\n", Itr);
}

physaddr_t physmem_allocblock()
{
  /* Get spinlock */
  physaddr_t addr = 0;
  interrupt_status_t intr_status = _interrupt_disable();
  spinlock_acquire(physmem_lock);

  /* Sanity */
  if(used_blocks >= total_blocks)
    {
      /* PANIC AT THE DISCO ! */
      KERNEL_PANIC("Physical Manager >> OUT OF MEMORY");
    }

  /* Get a frame */
  int64_t frame = physmem_getframe();

  if(frame == -1)
    {
      /* PANIC AT THE DISCO ! */
      spinlock_release(physmem_lock);
      KERNEL_PANIC("Physical Manager >> OUT OF MEMORY");
    }

  /* Mark it used */
  memmap_setbit(frame);

  /* Release spinlock */
  spinlock_release(physmem_lock);
  _interrupt_set_state(intr_status);

  /* Calculate Address */
  addr = (physaddr_t)(frame * PMM_BLOCK_SIZE);
  used_blocks++;

  return addr;
}

void physmem_freeblock(void *ptr)
{
  /* Calculate frame */
  uint64_t addr = (uint64_t)ptr;
  int64_t frame = (int64_t)(addr / PMM_BLOCK_SIZE);

  /* Get lock */
  interrupt_status_t intr_status = _interrupt_disable();
  spinlock_acquire(physmem_lock);

  /* Free */
  memmap_unsetbit(frame);

  /* Release spinlock */
  spinlock_release(physmem_lock);
  _interrupt_set_state(intr_status);

  /* Stats */
  used_blocks--;
}

physaddr_t physmem_allocblocks(uint32_t count)
{
  /* Get spinlock */
  physaddr_t addr = 0, i;
  interrupt_status_t intr_status = _interrupt_disable();
  spinlock_acquire(physmem_lock);

  /* Sanity */
  if(used_blocks >= total_blocks)
    {
      /* PANIC AT THE DISCO ! */
      KERNEL_PANIC("Physical Manager >> OUT OF MEMORY");
    }

  /* Get a frame */
  int64_t frame = physmem_getframes(count);

  if(frame == -1)
    {
      /* PANIC AT THE DISCO ! */
      spinlock_release(physmem_lock);
      KERNEL_PANIC("Physical Manager >> OUT OF MEMORY");
    }

  /* Mark it used */
  for(i = 0; i < count; i++)
    memmap_setbit(frame + i);

  /* Release spinlock */
  spinlock_release(physmem_lock);
  _interrupt_set_state(intr_status);

  /* Calculate Address */
  addr = (uint64_t)(frame * PMM_BLOCK_SIZE);
  used_blocks += count;

  return addr;
}

void physmem_freeblocks(void *ptr, uint32_t size)
{
  /* Calculate frame */
  uint64_t addr = (uint64_t)ptr, i;
  int64_t frame = (int64_t)(addr / PMM_BLOCK_SIZE);

  /* Get lock */
  interrupt_status_t intr_status = _interrupt_disable();
  spinlock_acquire(physmem_lock);

  /* Free */
  for(i = 0; i < size; i++)
    memmap_unsetbit(frame + i);

  /* Release spinlock */
  spinlock_release(physmem_lock);
  _interrupt_set_state(intr_status);

  /* Stats */
  used_blocks -= size;
}

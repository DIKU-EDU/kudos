/*
 * Memory Management.
 */

#ifndef __KUDOS_MEMORY_H__
#define __KUDOS_MEMORY_H__

/* Includes */
#include "lib/types.h"
#include <mem.h>
#include <pagetable.h>

/* Physical Memory Management */
/* For the architecture that supports it */

/* Prototypes */
void physmem_init(void *boot_info);

physaddr_t physmem_allocblock();
physaddr_t physmem_allocblocks(uint32_t count);

void physmem_freeblock(void *ptr);
void physmem_freeblocks(void *ptr, uint32_t size);

/* Virtual Memory Management */
#define USERLAND_STACK_TOP 0x7fffeffc

/* For the architecture that supports it */
void vm_init(void);

void vm_map(pagetable_t *pagetable, physaddr_t physaddr,
            virtaddr_t vaddr, int flags);
void vm_unmap(pagetable_t *pagetable, virtaddr_t vaddr);

physaddr_t vm_getmap(pagetable_t *pagetable, virtaddr_t vaddr);
void vm_set_dirty(pagetable_t *pagetable, virtaddr_t vaddr, int dirty);

pagetable_t *vm_create_pagetable(uint32_t asid);
void vm_destroy_pagetable(pagetable_t *pagetable);
void vm_update_mappings(virtaddr_t *thread);

void vm_memwrite(pagetable_t *pagetable, unsigned int buflen,
                 virtaddr_t target, const void *source);

/* Kernel Heap Memory Management
 * Provides dynamic memory allocation */

/* Heap Structure - Linked List */
typedef struct heap_header_t
{
  /* Address */
  virtaddr_t addr;

  /* Flags */
  uint32_t flags;

  /* Length of this block */
  uint32_t length;

  /* Next link */
  struct heap_header_t *link;

} heap_header_t;

/* Defines */
#define MM_HEAP_LOCATION        0x10000000
#define MM_HEAP_ADDR_OFFSET     0x200000
#define MM_HEAP_SIZE            (0x20000000 - (MM_HEAP_LOCATION + MM_HEAP_ADDR_OFFSET))
#define MM_HEAP_END             0x20000000

#define MM_HEAP_FLAG_ALLOCATED  0x1

void heap_init(void);
virtaddr_t *kmalloc(uint32_t length);
void kfree(void *ptr);

#endif

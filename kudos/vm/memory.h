/*
 * Memory Management.
 */

#ifndef KUDOS_VM_MEMORY_H
#define KUDOS_VM_MEMORY_H

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
#define USERLAND_STACK_TOP 0xFFFFFFFFFFFFFFFF

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

void* kmalloc(uint64_t size);

//void vm_memwrite(pagetable_t *pagetable, unsigned int buflen,
//                 virtaddr_t target, const void *source);

void* kmalloc(uint64_t size);
void kfree(void* ptr);

#endif // KUDOS_VM_MEMORY_H

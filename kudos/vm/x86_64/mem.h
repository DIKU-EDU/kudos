/*
 * Internal Memory Configuration
 */

#ifndef KUDOS_VM_X86_64_MEM_H
#define KUDOS_VM_X86_64_MEM_H

#include "lib/types.h"
#include <pagetable.h>

/* Dummy defines */
#define ADDR_KERNEL_TO_PHYS(x) x
#define ADDR_PHYS_TO_KERNEL(x) x

#define PMM_BLOCK_SIZE 0x1000

/* Page table pool size */
#define VM_PTP_SIZE 1024 //4mb

/* Multiboot Memory Map Structure */
enum memmap_types_t
  {
    MEMTYPE_FREE = 1,   /* This is for us */
    MEMTYPE_RESERVED,   /* Memory we cannot use */
    MEMTYPE_RECLAIM,    /* This is the ACPI memory space */
    MEMTYPE_NVRAM       /* ACPI NVS Memory - Let it be */
  };

typedef struct memory_region
{
  uint32_t size;
  uint64_t base_address;
  uint64_t length;
  uint32_t type;
} __attribute__((packed)) mem_region_t;

void vmm_setcr3(uint64_t pdbr);
pagetable_t* vmm_get_kernel_pml4();

#endif // KUDOS_VM_X86_64_MEM_H

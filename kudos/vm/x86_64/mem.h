/*
 * Internal Memory Configuration
 */

#ifndef __MEM_H__
#define __MEM_H__

#include "lib/types.h"
#include <pagetable.h>

/* Dummy defines */
#define ADDR_KERNEL_TO_PHYS(x) x
#define ADDR_PHYS_TO_KERNEL(x) x

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
uint64_t vmm_get_kernel_pml4();
pml4_t *vmm_get_kernel_vmem();

#endif

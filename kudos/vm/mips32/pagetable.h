#ifndef KUDOS_VM_MIPS32_PAGETABLE_H
#define KUDOS_VM_MIPS32_PAGETABLE_H

#include "lib/libc.h"
#include <tlb.h>

/* Check whether given (virtual) address is even or odd mapping
   in a pair of mappings for TLB. */
#define ADDR_IS_ON_ODD_PAGE(addr)  ((addr) & 0x00001000)
#define ADDR_IS_ON_EVEN_PAGE(addr) (!((addr) & 0x00001000))

/* Number of mapping entries in one pagetable. This is the number
   of entries that fits on a single hardware memory page (4k). */
#define PAGETABLE_ENTRIES 340

/* A pagetable. This structure fits on one physical page (4k). */
typedef struct pagetable_struct_t{
    /* Address space identifier. We use Thread Ids in KUDOS. */
    uint32_t ASID;
    /* Number of valid consecutive mappings in this pagetable. */
    uint32_t valid_count;
    /* Actual virtual memory mapping entries*/
    tlb_entry_t entries[PAGETABLE_ENTRIES];
} pagetable_t;

#endif // KUDOS_VM_MIPS32_PAGETABLE_H

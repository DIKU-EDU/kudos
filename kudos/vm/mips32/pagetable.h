/*
 * Pagetable.
 */

#ifndef KUDOS_VM_PAGETABLE_H
#define KUDOS_VM_PAGETABLE_H

#include "lib/libc.h"
#include <tlb.h>

/* Number of mapping entries in one pagetable. This is the number
   of entries that fits on a single hardware memory page (4k). */
#define PAGETABLE_ENTRIES 340

/* A pagetable. This structure fits on one physical page (4k). */
typedef struct pagetable_struct_t{
    /* Address space identifier. We use Thread Ids in Buenos. */
    uint32_t ASID;
    /* Number of valid consecutive mappings in this pagetable. */
    uint32_t valid_count;
    /* Actual virtual memory mapping entries*/
    tlb_entry_t entries[PAGETABLE_ENTRIES];
} pagetable_t;


#endif /* KUDOS_VM_PAGETABLE_H */

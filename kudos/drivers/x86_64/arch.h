/*
 * x86_64 specific constants
 */

#ifndef KUDOS_DRIVERS_X86_64_ARCH_H
#define KUDOS_DRIVERS_X86_64_ARCH_H

#include "lib/types.h"

/* Page size */
#define PAGE_SIZE 4096

/* Page portion of any address */
#define PAGE_SIZE_MASK 0xFFFFFFFFFFFFF000

/* Offset into page of any address */
#define PAGE_OFFSET_MASK (~PAGE_SIZE_MASK)

/* The structure of X64 device descriptor. */
typedef struct {
    /* Not used */
    uint32_t type          __attribute__ ((packed)); 

} io_descriptor_t;

#endif // KUDOS_DRIVERS_X86_64_ARCH_H

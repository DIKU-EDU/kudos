/*
 * x86_64 specific constants
 */

#ifndef DRIVERS_X86_H
#define DRIVERS_X86_H

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

#endif

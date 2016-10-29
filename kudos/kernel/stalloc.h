/*
 * Kernel static memory allocation.
 */

#ifndef KUDOS_KERNEL_STALLOC_H
#define KUDOS_KERNEL_STALLOC_H

#include "vm/memory.h"

void stalloc_disable();

/* Initialize the memory allocator */
void stalloc_init(void);

/* Permanent kernel memory allocation */
physaddr_t *stalloc(int bytes);

#endif // KUDOS_KERNEL_STALLOC_H

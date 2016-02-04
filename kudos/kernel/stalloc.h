/*
 * Kernel static memory allocation.
 */

#ifndef STALLOC_H
#define STALLOC_H

#include "vm/memory.h"

void stalloc_disable();

/* Initialize the memory allocator */
void stalloc_init(void);

/* Permanent kernel memory allocation */
physaddr_t *stalloc(int bytes);

#endif

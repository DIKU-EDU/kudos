/*
 * Internal Memory Configuration
 */

#ifndef KUDOS_VM_MIPS32_MEM_H
#define KUDOS_VM_MIPS32_MEM_H

#include "lib/types.h"

#define ADDR_PHYS_TO_KERNEL(addr) ((addr) | 0x80000000)
#define ADDR_KERNEL_TO_PHYS(addr) ((addr) & 0x7fffffff)

#endif // KUDOS_VM_MIPS32_MEM_H

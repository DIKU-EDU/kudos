/*
 * Internal Memory Configuration
 */

#ifndef __MEM_H__
#define __MEM_H__

#include "lib/types.h"

#define ADDR_PHYS_TO_KERNEL(addr) ((addr) | 0x80000000)
#define ADDR_KERNEL_TO_PHYS(addr) ((addr) & 0x7fffffff)

#endif /* __MEM_H__ */

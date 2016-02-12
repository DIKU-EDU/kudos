/*
 * Context switch.
 */
#ifndef MIPS_ASM_H
#define MIPS_ASM_H

#include "lib/registers.h"

/* Get cpu number into register reg. */
#define _FETCH_CPU_NUM(reg)                     \
  mfc0  reg, PRId, 0;                           \
  srl   reg, reg, 24;

#endif /* KERNEL_ASM_H */

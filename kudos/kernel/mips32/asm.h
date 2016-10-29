/*
 * Context switch.
 */
#ifndef KUDOS_KERNEL_MIPS32_ASM_H
#define KUDOS_KERNEL_MIPS32_ASM_H

#include "lib/registers.h"

/* Get cpu number into register reg. */
#define _FETCH_CPU_NUM(reg)                     \
  mfc0  reg, PRId, 0;                           \
  srl   reg, reg, 24;

#endif // KUDOS_KERNEL_MIPS32_ASM_H

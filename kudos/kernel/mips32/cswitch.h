/*
 * Context switch.
 */

#ifndef KUDOS_KERNEL_MIPS32_CSWITCH_H
#define KUDOS_KERNEL_MIPS32_CSWITCH_H

#include "lib/types.h"
#include "vm/memory.h"

/* Indexes of registers in the register table in context_t structure */
#define MIPS_REGISTER_AT 0
#define MIPS_REGISTER_V0 1
#define MIPS_REGISTER_V1 2
#define MIPS_REGISTER_A0 3
#define MIPS_REGISTER_A1 4
#define MIPS_REGISTER_A2 5
#define MIPS_REGISTER_A3 6
#define MIPS_REGISTER_T0 7
#define MIPS_REGISTER_T1 8
#define MIPS_REGISTER_T2 9
#define MIPS_REGISTER_T3 10
#define MIPS_REGISTER_T4 11
#define MIPS_REGISTER_T5 12
#define MIPS_REGISTER_T6 13
#define MIPS_REGISTER_T7 14
#define MIPS_REGISTER_S0 15
#define MIPS_REGISTER_S1 16
#define MIPS_REGISTER_S2 17
#define MIPS_REGISTER_S3 18
#define MIPS_REGISTER_S4 19
#define MIPS_REGISTER_S5 20
#define MIPS_REGISTER_S6 21
#define MIPS_REGISTER_S7 22
#define MIPS_REGISTER_T8 23
#define MIPS_REGISTER_T9 24
#define MIPS_REGISTER_GP 25
#define MIPS_REGISTER_SP 26
#define MIPS_REGISTER_FP 27
#define MIPS_REGISTER_RA 28

#define USERLAND_ENABLE_BIT 0x00000010

/* Thread context data structure */
typedef struct {
  uint32_t cpu_regs[29];   /* The general purpose registers. zero, k0 and 
                              k1 registers are omitted. */
  uint32_t hi;             /* The hi register. */
  uint32_t lo;             /* The lo register. */
  uint32_t pc;             /* The program counter. Actually loaded from 
                              EPC register in co-processor 0. */
  uint32_t status;         /* Status register bits. */
  void    *prev_context;   /* Previous context in a nested exception chain */
} context_t;

/* Code to be inserted to interrupt vector */
void _cswitch_vector_code(void);

/* Userland entering code */
void _cswitch_to_userland(context_t *usercontext);

/* Context manipulation */
void _context_init(context_t *cxt, virtaddr_t entry, virtaddr_t endentry, 
                   virtaddr_t stack, uint32_t args);
void _context_enter_userland(context_t *cxt);
void _context_set_ip(context_t *cxt, virtaddr_t ip); /* Set new instruction pointer / program counter */
void _context_set_sp(context_t *cxt, virtaddr_t sp); /* Sets a new stack pointer */
void _context_enable_ints(context_t *cxt); /* Enables interrupts */

#endif // KUDOS_KERNEL_MIPS32_CSWITCH_H

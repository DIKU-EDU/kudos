/*
 * Context switch.
 */
#ifndef KUDOS_KERNEL_X86_64_CSWITCH_H
#define KUDOS_KERNEL_X86_64_CSWITCH_H

#include "lib/types.h"
#include "vm/memory.h"

/* Thread context data structure */
typedef struct {
  uint64_t *stack;   /* The stack */
  uint64_t rip;
  uint64_t flags;
  uint64_t pml4;
  pagetable_t *virt_memory;

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
void _context_enable_ints(context_t *cxt); /* Masks interrupts */

#endif // KUDOS_KERNEL_X86_64_CSWITCH_H

/*
 * Context switch.
 */

#include <cswitch.h>
#include "kernel/interrupt.h"

/* Setup new thread context */
void _context_init(context_t *cxt, virtaddr_t entry,
                   virtaddr_t endentry, virtaddr_t stack, uint32_t args)
{
  /* Setup SP and IP */
  cxt->pc = entry;
  cxt->cpu_regs[MIPS_REGISTER_SP] = stack - 4;  /* Reserve space for arg */
  cxt->cpu_regs[MIPS_REGISTER_A0] = args;
  cxt->cpu_regs[MIPS_REGISTER_RA] = endentry;
  cxt->status = INTERRUPT_MASK_ALL | INTERRUPT_MASK_MASTER;
}

void _context_enter_userland(context_t *cxt)
{
  /* Set userland bit and enable interrupts before entering userland. */
  cxt->status = cxt->status | USERLAND_ENABLE_BIT;
  cxt->status = cxt->status | INTERRUPT_MASK_ALL;
  cxt->status = cxt->status | INTERRUPT_MASK_MASTER;
  _cswitch_to_userland(cxt);
}

void _context_set_ip(context_t *cxt, virtaddr_t ip)
{
  /* Set pc */
  cxt->pc = ip;
}

void _context_enable_ints(context_t *cxt)
{
  /* Modify status */
  cxt->status = INTERRUPT_MASK_ALL | INTERRUPT_MASK_MASTER;
}

void _context_set_sp(context_t *cxt, virtaddr_t sp)
{
  cxt->cpu_regs[MIPS_REGISTER_SP] = sp;
}

/*
 * Function Stubs
 */
#include "kernel/interrupt.h"
#include "lib/libc.h"

int _interrupt_getcpu()
{
  return 0;
}

void shutdown(int err)
{
  /* Print */
  kprintf("System shutdown called, it is not implemented\n");
  kprintf("However it is safe to shutdown the computer now.");
  while(1);
  return;
}
